//! Mip level filtering for RGBA8 textures.

// The chunk sizes are constants, but several are generic over the float count,
// where `as_chunks` does not apply, so one style reads better than two.
#![allow(clippy::chunks_exact_to_as_chunks)]

use rayon::prelude::*;
use std::sync::LazyLock;

const CHANNELS: usize = 4;

/// Floats per texel while filtering. Opaque or linear data needs R, G, B and
/// A. Colour with transparent texels also carries the plain R, G and B beside
/// the alpha weighted ones, as R G B A Rw Gw Bw Aw. Where the weighted alpha
/// is 0 only the plain colour has a value, and the engine draws albedo without
/// alpha, so dropping it would turn foliage cards black.
const SINGLE: usize = CHANNELS;
const DOUBLE: usize = CHANNELS * 2;

/// Below this the weighted alpha is rounding noise and the plain colour is
/// used instead.
const MIN_ALPHA: f32 = 1.0e-6;

/// A table converts linear light to an sRGB byte, looked up by
/// `linear * SRGB_STEPS`, because `powf` per texel costs more than the whole
/// filter. The steps are fine enough that a lookup is within one level of the
/// exact curve, even in the darks where the curve is steep.
const SRGB_STEPS: usize = 16384;

/// How the channels of a mip level are averaged.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum MipSpace {
    /// sRGB colour: filtered in linear light and weighted by alpha, so
    /// transparent texels add no colour. Alpha is filtered as it is.
    Srgb,
    /// Every channel is data, alpha included, with no decode and no weighting.
    Linear,
}

/// Mitchell-Netravali, B = C = 1/3. The weight is 0 from 2 on.
fn mitchell(x: f32) -> f32 {
    let x = x.abs();
    if x < 1.0 {
        (16.0 + x * x * (21.0 * x - 36.0)) / 18.0
    } else if x < 2.0 {
        (32.0 + x * (-60.0 + x * (36.0 - 7.0 * x))) / 18.0
    } else {
        0.0
    }
}

/// The source texels that make one output texel, as a run from `first`.
#[derive(Default)]
struct Taps {
    first: usize,
    weights: Vec<f32>,
}

fn make_taps(src_size: u32, dst_size: u32) -> Vec<Taps> {
    // Only a one pixel axis keeps its size. A filter would blur it.
    if src_size == dst_size {
        return (0..dst_size as usize)
            .map(|i| Taps {
                first: i,
                weights: vec![1.0],
            })
            .collect();
    }

    let scale = src_size as f32 / dst_size as f32;
    let radius = 2.0 * scale;
    let last = i64::from(src_size) - 1;

    (0..dst_size)
        .map(|i| {
            let centre = (i as f32 + 0.5) * scale;
            let lo = (centre - radius).floor() as i64;
            let hi = (centre + radius).ceil() as i64;
            let first = lo.clamp(0, last);
            let mut weights = vec![0.0f32; (hi.clamp(0, last) - first + 1) as usize];

            // Taps past the border use the edge texel.
            let mut sum = 0.0f32;
            for k in lo..=hi {
                let weight = mitchell((k as f32 + 0.5 - centre) / scale);
                weights[(k.clamp(0, last) - first) as usize] += weight;
                sum += weight;
            }
            for weight in &mut weights {
                *weight /= sum;
            }
            Taps {
                first: first as usize,
                weights,
            }
        })
        .collect()
}

static SRGB_TO_LINEAR: LazyLock<[f32; 256]> = LazyLock::new(|| {
    std::array::from_fn(|i| {
        let v = i as f32 / 255.0;
        if v <= 0.04045 {
            v / 12.92
        } else {
            ((v + 0.055) / 1.055).powf(2.4)
        }
    })
});

fn to_byte(v: f32) -> u8 {
    ((v.clamp(0.0, 1.0) * 255.0) + 0.5) as u8
}

static LINEAR_TO_SRGB: LazyLock<Vec<u8>> = LazyLock::new(|| {
    (0..=SRGB_STEPS)
        .map(|i| {
            let l = i as f32 / SRGB_STEPS as f32;
            to_byte(if l <= 0.003_130_8 {
                l * 12.92
            } else {
                1.055 * l.powf(1.0 / 2.4) - 0.055
            })
        })
        .collect()
});

/// Source row to floats. Colour is decoded and multiplied by alpha, and kept
/// plain as well when there are two sets of channels.
fn decode_row<const F: usize>(src: &[u8], space: MipSpace, out: &mut [f32]) {
    const BYTE_SCALE: f32 = 1.0 / 255.0;

    if space == MipSpace::Srgb {
        let lut = &*SRGB_TO_LINEAR;
        for (texel_in, dst) in src.chunks_exact(CHANNELS).zip(out.chunks_exact_mut(F)) {
            let alpha = f32::from(texel_in[3]) * BYTE_SCALE;
            let texel = [
                lut[usize::from(texel_in[0])],
                lut[usize::from(texel_in[1])],
                lut[usize::from(texel_in[2])],
                1.0,
            ];
            if F == DOUBLE {
                dst[..CHANNELS].copy_from_slice(&[texel[0], texel[1], texel[2], alpha]);
            }
            for c in 0..CHANNELS {
                dst[F - CHANNELS + c] = texel[c] * alpha;
            }
        }
        return;
    }

    for (dst, &byte) in out.iter_mut().zip(src) {
        *dst = f32::from(byte) * BYTE_SCALE;
    }
}

/// Output row back to bytes. Colour is divided by alpha and encoded. Where
/// alpha is 0 the plain colour is used, if there is one.
fn encode_row<const F: usize>(src: &[f32], space: MipSpace, out: &mut [u8]) {
    if space == MipSpace::Srgb {
        let table = &*LINEAR_TO_SRGB;
        for (texel, dst) in src.chunks_exact(F).zip(out.chunks_exact_mut(CHANNELS)) {
            let alpha = texel[3].clamp(0.0, 1.0);
            for c in 0..3 {
                let unit = if alpha >= MIN_ALPHA {
                    texel[F - CHANNELS + c] * (1.0 / alpha)
                } else if F == DOUBLE {
                    texel[c]
                } else {
                    0.0
                };
                dst[c] = table[((unit.clamp(0.0, 1.0) * SRGB_STEPS as f32) + 0.5) as usize];
            }
            dst[3] = to_byte(alpha);
        }
        return;
    }

    for (dst, &value) in out.iter_mut().zip(src) {
        *dst = to_byte(value);
    }
}

/// One source row, filtered across its width.
fn filter_row<const F: usize>(decoded: &[f32], taps: &[Taps], out: &mut [f32]) {
    for (tap, dst) in taps.iter().zip(out.chunks_exact_mut(F)) {
        let input = &decoded[tap.first * F..];
        let mut acc = [0.0f32; F];
        for (k, &weight) in tap.weights.iter().enumerate() {
            for (i, sum) in acc.iter_mut().enumerate() {
                *sum += input[k * F + i] * weight;
            }
        }
        dst.copy_from_slice(&acc);
    }
}

fn has_transparency(pixels: &[u8]) -> bool {
    pixels.chunks_exact(CHANNELS).any(|texel| texel[3] != 255)
}

/// Output rows `y_begin..y_end` of a halved image. Source rows are filtered
/// across once, in order, and kept in a circular buffer just big enough for the widest
/// vertical window. Whole image intermediates would cost hundreds of MB on a
/// 4K texture.
fn halve_band<const F: usize>(
    pixels: &[u8],
    width: usize,
    space: MipSpace,
    columns: &[Taps],
    rows: &[Taps],
    y_range: std::ops::Range<usize>,
) -> Vec<u8> {
    let dst_width = columns.len();
    let window = rows.iter().map(|row| row.weights.len()).max().unwrap_or(1);
    let stride = dst_width * F;

    let mut ring = vec![0.0f32; window * stride];
    let mut line = vec![0.0f32; stride];
    let mut decoded = vec![0.0f32; width * F];
    let mut out = vec![0u8; y_range.len() * dst_width * CHANNELS];

    let mut filtered = rows[y_range.start].first;
    for y in y_range.clone() {
        let tap = &rows[y];

        while filtered < tap.first + tap.weights.len() {
            let row = &pixels[filtered * width * CHANNELS..][..width * CHANNELS];
            decode_row::<F>(row, space, &mut decoded);
            let slot = (filtered % window) * stride;
            filter_row::<F>(&decoded, columns, &mut ring[slot..slot + stride]);
            filtered += 1;
        }

        for (i, value) in line.iter_mut().enumerate() {
            let mut acc = 0.0f32;
            for (k, &weight) in tap.weights.iter().enumerate() {
                acc += weight * ring[((tap.first + k) % window) * stride + i];
            }
            *value = acc;
        }

        let row = (y - y_range.start) * dst_width * CHANNELS;
        encode_row::<F>(&line, space, &mut out[row..row + dst_width * CHANNELS]);
    }
    out
}

fn halve<const F: usize>(pixels: &[u8], width: u32, height: u32, space: MipSpace) -> Vec<u8> {
    let dst_width = (width / 2).max(1);
    let dst_height = ((height / 2).max(1)) as usize;
    let columns = make_taps(width, dst_width);
    let rows = make_taps(height, dst_height as u32);

    // Rows below this many per band are not worth a thread.
    const MIN_BAND_ROWS: usize = 64;
    let bands = (dst_height / MIN_BAND_ROWS).max(1);

    // Output rows are independent, so each band is filtered on its own
    // thread. A band starts by filtering the few source rows its first window
    // needs, which its neighbour also filters, so the bytes are the same as one
    // pass over the whole image.
    (0..bands)
        .into_par_iter()
        .map(|band| {
            let range = dst_height * band / bands..dst_height * (band + 1) / bands;
            halve_band::<F>(pixels, width as usize, space, &columns, &rows, range)
        })
        .flatten_iter()
        .collect()
}

/// Halves an RGBA8 image to `max(width / 2, 1)` by `max(height / 2, 1)` with a
/// Mitchell filter (B = C = 1/3) stretched to the scale ratio, so odd sizes
/// need no special case. Edge texels repeat past the border.
pub fn halve_mip(pixels: &[u8], width: u32, height: u32, space: MipSpace) -> Vec<u8> {
    assert_eq!(pixels.len(), width as usize * height as usize * CHANNELS);
    // Only colour with transparent texels needs the second set of channels.
    if space == MipSpace::Srgb && has_transparency(pixels) {
        halve::<DOUBLE>(pixels, width, height, space)
    } else {
        halve::<SINGLE>(pixels, width, height, space)
    }
}

// The alpha test of a glTF MASK material keeps a texel when alpha >= cutoff.
// Averaging thins a mip's alpha, so fewer texels pass at each level and the
// foliage fades away with distance. The two functions below keep the share of
// passing texels the same at every level.
//
// `threshold` is the cutoff as an 8 bit alpha value, 1 to 255: the smallest
// stored alpha that passes.

/// Fraction of texels, 0 to 1, that pass the alpha test.
pub fn alpha_coverage(pixels: &[u8], threshold: u8) -> f64 {
    let texels = pixels.len() / CHANNELS;
    if texels == 0 {
        return 0.0;
    }
    let passing = pixels
        .chunks_exact(CHANNELS)
        .filter(|texel| texel[3] >= threshold)
        .count();
    passing as f64 / texels as f64
}

/// Scales the alpha of a mip so that the fraction passing the test is as close
/// to `target` as 8 bits allow, never below it. Colour is left alone. Does
/// nothing when `target` is 0 or 1: nothing, or everything, passes at every
/// scale.
pub fn scale_alpha_coverage(pixels: &mut [u8], threshold: u8, target: f64) {
    let texels = pixels.len() / CHANNELS;
    if texels == 0 || target <= 0.0 || target >= 1.0 {
        return;
    }

    let mut histogram = [0usize; 256];
    for texel in pixels.chunks_exact(CHANNELS) {
        histogram[usize::from(texel[3])] += 1;
    }

    // The largest alpha that, taken as the new cutoff, still leaves enough
    // texels passing. Scaling it up to the real threshold does the same to
    // every alpha.
    let need = (target * texels as f64).ceil() as usize;
    let mut passing = 0;
    let mut t = 255;
    while t > 1 {
        passing += histogram[t];
        if passing >= need {
            break;
        }
        t -= 1;
    }
    let scale = f64::from(threshold) / t as f64;
    for texel in pixels.chunks_exact_mut(CHANNELS) {
        texel[3] = (f64::from(texel[3]) * scale).round().min(255.0) as u8;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn solid(width: u32, height: u32, texel: [u8; 4]) -> Vec<u8> {
        texel.repeat((width * height) as usize)
    }

    #[test]
    fn sizes_halve_and_stop_at_one() {
        for (w, h, dw, dh) in [(8, 8, 4, 4), (7, 5, 3, 2), (4, 1, 2, 1), (1, 1, 1, 1)] {
            let out = halve_mip(&solid(w, h, [10, 20, 30, 255]), w, h, MipSpace::Linear);
            assert_eq!(out.len(), (dw * dh * 4) as usize, "{w}x{h}");
        }
    }

    #[test]
    fn a_flat_color_stays_flat() {
        for space in [MipSpace::Srgb, MipSpace::Linear] {
            let out = halve_mip(&solid(16, 16, [200, 100, 50, 255]), 16, 16, space);
            for texel in out.chunks_exact(4) {
                assert_eq!(texel, [200, 100, 50, 255], "{space:?}");
            }
        }
    }

    #[test]
    fn srgb_averages_in_linear_light() {
        // Black and white average to a mid grey in linear light, which is
        // much brighter once encoded than the plain byte average.
        let mut pixels = Vec::new();
        for x in 0..4 {
            pixels.extend_from_slice(if x % 2 == 0 {
                &[0, 0, 0, 255]
            } else {
                &[255, 255, 255, 255]
            });
        }
        let srgb = halve_mip(&pixels, 4, 1, MipSpace::Srgb);
        let linear = halve_mip(&pixels, 4, 1, MipSpace::Linear);
        assert!(
            srgb[0] > linear[0] + 20,
            "{} against {}",
            srgb[0],
            linear[0]
        );
    }

    #[test]
    fn transparent_texels_add_no_color() {
        // Left half opaque red, right half clear green.
        let mut pixels = Vec::new();
        for x in 0..8 {
            pixels.extend_from_slice(if x < 4 {
                &[255, 0, 0, 255]
            } else {
                &[0, 255, 0, 0]
            });
        }
        let out = halve_mip(&pixels, 8, 1, MipSpace::Srgb);
        // The first output texel sees only red.
        assert_eq!(&out[..4], [255, 0, 0, 255]);
        // The last sees only clear texels, so the plain colour stays.
        assert_eq!(&out[12..16], [0, 255, 0, 0]);
    }

    #[test]
    fn a_tall_image_uses_bands_and_matches_one_pass() {
        let (w, h) = (6u32, 300u32);
        let pixels: Vec<u8> = (0..w * h * 4).map(|i| (i * 31 % 251) as u8).collect();
        let whole = halve_mip(&pixels, w, h, MipSpace::Linear);
        let columns = make_taps(w, w / 2);
        let rows = make_taps(h, h / 2);
        let one = halve_band::<SINGLE>(
            &pixels,
            w as usize,
            MipSpace::Linear,
            &columns,
            &rows,
            0..(h / 2) as usize,
        );
        assert_eq!(whole, one);
    }

    #[test]
    fn coverage_is_kept() {
        let mut pixels: Vec<u8> = (0..100u32)
            .flat_map(|i| [0, 0, 0, (i * 255 / 99) as u8])
            .collect();
        let target = alpha_coverage(&pixels, 128);
        // A thinner mip: every alpha scaled down.
        for texel in pixels.chunks_exact_mut(4) {
            texel[3] = (f64::from(texel[3]) * 0.7) as u8;
        }
        assert!(alpha_coverage(&pixels, 128) < target);
        scale_alpha_coverage(&mut pixels, 128, target);
        assert!(alpha_coverage(&pixels, 128) >= target);
    }

    #[test]
    fn coverage_scaling_ignores_all_or_nothing() {
        let mut pixels = solid(2, 2, [0, 0, 0, 100]);
        scale_alpha_coverage(&mut pixels, 128, 0.0);
        scale_alpha_coverage(&mut pixels, 128, 1.0);
        assert_eq!(pixels, solid(2, 2, [0, 0, 0, 100]));
    }
}

/// Reads RGBA8 files named by `YESCHEF_MIP_IN` (a folder of `<w>x<h>.rgba`
/// raw files) and writes both halvings beside them, to compare with the C++
/// filter. Run with `cargo test dump_mips -- --ignored`.
#[cfg(test)]
#[test]
#[ignore]
fn dump_mips() {
    let dir = std::path::PathBuf::from(std::env::var("YESCHEF_MIP_IN").unwrap());
    for entry in std::fs::read_dir(&dir).unwrap() {
        let path = entry.unwrap().path();
        let Some(name) = path.file_stem().and_then(|n| n.to_str()) else {
            continue;
        };
        if path.extension().is_none_or(|e| e != "rgba") || name.contains('.') {
            continue;
        }
        let (w, h) = name.split_once('x').unwrap();
        let (w, h): (u32, u32) = (w.parse().unwrap(), h.parse().unwrap());
        let pixels = std::fs::read(&path).unwrap();
        for (tag, space) in [("srgb", MipSpace::Srgb), ("linear", MipSpace::Linear)] {
            let out = halve_mip(&pixels, w, h, space);
            std::fs::write(dir.join(format!("{name}.{tag}.rust")), out).unwrap();
        }
    }
}
