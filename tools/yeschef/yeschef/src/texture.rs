//! Compresses an image to BC7 or BC5 with a full mip chain, as a KTX2 file.

use anyhow::{Context, Result, bail};
use ctt_bc7f::bindings::cPackBC7FlagDefaultPartiallyAnalytical;
use rayon::prelude::*;
use std::path::Path;
use yeschef_format::ktx2::{self, Format};

use crate::image::{self, Rgba};
use crate::mip::{self, MipSpace};

const BLOCK: u32 = 4;
const TEXELS_IN_BLOCK: usize = (BLOCK * BLOCK) as usize;
const BC7_BLOCK_BYTES: usize = 16;
const BC4_BLOCK_BYTES: usize = 8;
const CHANNELS: usize = 4;

/// BC7F encoder flags. Measured on the six largest Sponza and helmet textures
/// against bc7enc, the encoder of the C++ converter this tool replaced
/// (39.35 dB mean, 5.5 s): `Default` gave 39.11 dB
/// in 1.0 s, this preset 39.88 dB in 1.2 s and `DefaultNonAnalytical` 40.32 dB
/// in 2.0 s. Take the non-analytical preset if bake time stops mattering.
const BC7_FLAGS: u32 = cPackBC7FlagDefaultPartiallyAnalytical;

/// What a texture is for. The kind decides the block format and whether the
/// mip chain is filtered in sRGB or linear space.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum TextureKind {
    /// Albedo and emissive: three or four channels through BC7.
    Color,
    /// Normal maps: XY through BC5, Z reconstructed in the shader.
    Normal,
    /// Occlusion and metallic-roughness: BC7, but resampled linearly.
    Linear,
}

/// A 4x4 block of RGBA8 texels in row order.
type Block = [u8; TEXELS_IN_BLOCK * CHANNELS];

/// One level of an RGBA8 image.
#[derive(Clone)]
struct Surface {
    width: u32,
    height: u32,
    pixels: Vec<u8>,
}

/// Copies one 4x4 block out of a surface. Blocks that extend past the right or
/// bottom edge repeat the last row and column rather than reading past the
/// buffer, and the decoder never samples those texels.
fn read_block(surface: &Surface, block_x: u32, block_y: u32) -> Block {
    let mut block = [0u8; TEXELS_IN_BLOCK * CHANNELS];
    for y in 0..BLOCK {
        let source_y = (block_y + y).min(surface.height - 1) as usize * surface.width as usize;
        for x in 0..BLOCK {
            let source_x = (block_x + x).min(surface.width - 1) as usize;
            let from = (source_y + source_x) * CHANNELS;
            let to = ((y * BLOCK + x) as usize) * CHANNELS;
            block[to..to + CHANNELS].copy_from_slice(&surface.pixels[from..from + CHANNELS]);
        }
    }
    block
}

/// One BC4 block: two endpoints plus sixteen 3 bit indices. Endpoint order
/// `red0 > red1` selects the eight value ramp, so index 0 decodes to `red0`,
/// index 1 to `red1`, and 2 to 7 to the six interpolated steps between them.
///
/// Indices come from a nearest search over the decoded ramp instead of an
/// arithmetic mapping. Eight comparisons per texel costs nothing here and
/// there is no index order to get wrong.
fn encode_bc4(block: &Block, channel: usize, out: &mut [u8]) {
    let mut values = [0u8; TEXELS_IN_BLOCK];
    for (i, value) in values.iter_mut().enumerate() {
        *value = block[i * CHANNELS + channel];
    }
    let low = *values.iter().min().unwrap();
    let high = *values.iter().max().unwrap();
    out[0] = high;
    out[1] = low;

    let mut ramp = [0i32; 8];
    ramp[0] = i32::from(high);
    ramp[1] = i32::from(low);
    for (i, step) in ramp.iter_mut().enumerate().skip(2) {
        *step = ((8 - i as i32) * i32::from(high) + (i as i32 - 1) * i32::from(low)) / 7;
    }

    let mut indices = 0u64;
    for (i, &value) in values.iter().enumerate() {
        let mut best = 0;
        let mut best_error = i32::MAX;
        for (candidate, &step) in ramp.iter().enumerate() {
            let error = (step - i32::from(value)).abs();
            if error < best_error {
                best_error = error;
                best = candidate;
            }
        }
        indices |= (best as u64) << (i * 3);
    }
    for (i, byte) in out[2..8].iter_mut().enumerate() {
        *byte = (indices >> (i * 8)) as u8;
    }
}

fn compress(surface: &Surface, kind: TextureKind) -> Vec<u8> {
    let blocks_x = surface.width.div_ceil(BLOCK) as usize;
    let blocks_y = surface.height.div_ceil(BLOCK) as usize;
    let normal = kind == TextureKind::Normal;
    let bytes_per_block = if normal {
        BC4_BLOCK_BYTES * 2
    } else {
        BC7_BLOCK_BYTES
    };

    let mut out = vec![0u8; blocks_x * blocks_y * bytes_per_block];
    // Blocks are independent, so rows go to the pool one at a time and give
    // the same bytes as a serial run. A fixed split left threads idle on rows
    // that encode fast.
    out.par_chunks_mut(blocks_x * bytes_per_block)
        .enumerate()
        .for_each(|(by, row)| {
            let blocks: Vec<Block> = (0..blocks_x as u32)
                .map(|bx| read_block(surface, bx * BLOCK, by as u32 * BLOCK))
                .collect();
            if normal {
                for (block, dst) in blocks.iter().zip(row.chunks_exact_mut(bytes_per_block)) {
                    encode_bc4(block, 0, &mut dst[..BC4_BLOCK_BYTES]);
                    encode_bc4(block, 1, &mut dst[BC4_BLOCK_BYTES..]);
                }
            } else {
                ctt_bc7f::compress_blocks_into(blocks.as_flattened(), row, BC7_FLAGS);
            }
        });
    out
}

/// Half size resample. Colour is filtered as sRGB, and everything else is data
/// that must be filtered as it is, or the mips shift in brightness.
fn halve(surface: &Surface, kind: TextureKind) -> Surface {
    let space = if kind == TextureKind::Color {
        MipSpace::Srgb
    } else {
        MipSpace::Linear
    };
    Surface {
        width: (surface.width / 2).max(1),
        height: (surface.height / 2).max(1),
        pixels: mip::halve_mip(&surface.pixels, surface.width, surface.height, space),
    }
}

fn format_for(kind: TextureKind) -> Format {
    // BC7 UNORM, not SRGB, to match the GL_RGBA8 the engine uploaded before
    // baking existed, so compression is the only change. Switching albedo to
    // sRGB is a separate, visible change.
    if kind == TextureKind::Normal {
        Format::Bc5Unorm
    } else {
        Format::Bc7Unorm
    }
}

/// Compresses an image and returns the whole KTX2 file, mip chain included.
///
/// `alpha_cutoff` is the alpha a texel needs to pass a glTF MASK test, 0 for
/// none. Each mip then has its alpha scaled so the share of passing texels
/// matches level 0, which keeps foliage from thinning with distance. Colour
/// only.
pub fn encode(image: &Rgba, kind: TextureKind, alpha_cutoff: f32) -> Result<Vec<u8>> {
    if image.width == 0 || image.height == 0 || image.pixels.is_empty() {
        bail!("cannot encode an empty image");
    }
    let mut surface = Surface {
        width: image.width,
        height: image.height,
        pixels: image.pixels.clone(),
    };

    // Full chain down to 1x1. Compressed levels smaller than a block still
    // occupy one block, which is what glCompressedTexImage2D expects.
    //
    // The next level is always filtered from the unscaled one, so the alpha
    // scale does not compound down the chain.
    let threshold = (alpha_cutoff * 255.0).ceil();
    let keep_coverage = kind == TextureKind::Color && (1.0..=255.0).contains(&threshold);
    let coverage = if keep_coverage {
        mip::alpha_coverage(&surface.pixels, threshold as u8)
    } else {
        0.0
    };

    let mut levels = Vec::new();
    loop {
        if keep_coverage && !levels.is_empty() {
            let mut scaled = surface.clone();
            mip::scale_alpha_coverage(&mut scaled.pixels, threshold as u8, coverage);
            levels.push(compress(&scaled, kind));
        } else {
            levels.push(compress(&surface, kind));
        }
        if surface.width == 1 && surface.height == 1 {
            break;
        }
        surface = halve(&surface, kind);
    }

    Ok(ktx2::write(
        format_for(kind),
        image.width,
        image.height,
        &levels,
        &[],
    ))
}

/// One image, BC7 with a full mip chain, for UI art that is too big to share
/// a sprite sheet, because a single large image forces the whole sheet up to
/// the next size.
pub fn convert(from: &Path, to: &Path) -> Result<()> {
    let bytes =
        std::fs::read(from).with_context(|| format!("unable to read {}", from.display()))?;
    let decoded =
        image::decode(&bytes).with_context(|| format!("unable to decode {}", from.display()))?;
    let file =
        encode(&decoded, TextureKind::Color, 0.0).with_context(|| from.display().to_string())?;
    std::fs::write(to, &file).with_context(|| format!("unable to write {}", to.display()))?;
    println!(
        "{} -> {} ({}x{}, {} bytes)",
        from.to_string_lossy().replace('\\', "/"),
        to.to_string_lossy().replace('\\', "/"),
        decoded.width,
        decoded.height,
        file.len()
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn solid(width: u32, height: u32, texel: [u8; 4]) -> Rgba {
        Rgba {
            width,
            height,
            pixels: texel.repeat((width * height) as usize),
        }
    }

    #[test]
    fn a_chain_runs_down_to_one_texel() {
        let file = encode(&solid(9, 5, [10, 20, 30, 255]), TextureKind::Color, 0.0).unwrap();
        let image = ktx2::read(&file).unwrap();
        assert_eq!(image.vk_format, 145);
        // 9x5, 4x2, 2x1, 1x1. Each level is at least one block.
        let sizes: Vec<usize> = image.levels.iter().map(|l| l.len()).collect();
        assert_eq!(sizes, [3 * 2 * 16, 16, 16, 16]);
    }

    #[test]
    fn normals_use_two_bc4_blocks() {
        let file = encode(&solid(8, 8, [200, 50, 255, 255]), TextureKind::Normal, 0.0).unwrap();
        let image = ktx2::read(&file).unwrap();
        assert_eq!(image.vk_format, 141);
        assert_eq!(image.levels[0].len(), 4 * 16);
    }

    #[test]
    fn bc4_decodes_back_to_the_block() {
        // A ramp the BC4 eight value mode can only approximate: its steps
        // are 34 apart, so no texel is more than 17 off.
        let mut block = [0u8; TEXELS_IN_BLOCK * CHANNELS];
        for i in 0..TEXELS_IN_BLOCK {
            block[i * CHANNELS] = (i * 16) as u8;
        }
        let mut out = [0u8; 8];
        encode_bc4(&block, 0, &mut out);
        let (high, low) = (i32::from(out[0]), i32::from(out[1]));
        assert!(high > low);
        let indices = out[2..]
            .iter()
            .enumerate()
            .fold(0u64, |acc, (i, &b)| acc | (u64::from(b) << (i * 8)));
        for i in 0..TEXELS_IN_BLOCK {
            let index = ((indices >> (i * 3)) & 7) as i32;
            let decoded = match index {
                0 => high,
                1 => low,
                n => ((8 - n) * high + (n - 1) * low) / 7,
            };
            let want = i32::from(block[i * CHANNELS]);
            assert!(
                (decoded - want).abs() <= 17,
                "texel {i}: {decoded} against {want}"
            );
        }
    }

    #[test]
    fn edge_blocks_repeat_the_last_texels() {
        let mut image = solid(5, 1, [0, 0, 0, 255]);
        image.pixels[16..20].copy_from_slice(&[9, 8, 7, 255]);
        let surface = Surface {
            width: 5,
            height: 1,
            pixels: image.pixels,
        };
        let block = read_block(&surface, 4, 0);
        // Every texel of the second block is the single texel at x = 4.
        for texel in block.as_chunks::<4>().0 {
            assert_eq!(*texel, [9, 8, 7, 255]);
        }
    }

    #[test]
    fn alpha_coverage_survives_the_chain() {
        // One texel in four is opaque and the rest are clear, so a quarter pass
        // a 128 cutoff. Filtering thins level 1 to an alpha near 64, which
        // would pass nothing; keeping coverage lifts it.
        let mut image = solid(16, 16, [90, 90, 90, 0]);
        for (i, texel) in image.pixels.as_chunks_mut::<4>().0.iter_mut().enumerate() {
            if i / 16 % 2 == 0 && i % 2 == 0 {
                texel[3] = 255;
            }
        }
        let with = encode(&image, TextureKind::Color, 0.5).unwrap();
        let without = encode(&image, TextureKind::Color, 0.0).unwrap();
        assert_eq!(with.len(), without.len());
        assert_ne!(with, without, "the scaled alpha changes the lower levels");
        // Level 0 is unscaled either way.
        let (a, b) = (ktx2::read(&with).unwrap(), ktx2::read(&without).unwrap());
        assert_eq!(a.levels[0], b.levels[0]);
    }

    #[test]
    fn an_empty_image_is_an_error() {
        let empty = Rgba {
            width: 0,
            height: 0,
            pixels: Vec::new(),
        };
        assert!(encode(&empty, TextureKind::Color, 0.0).is_err());
    }
}
