//! Packs several small images into one KTX2 sprite sheet. The rect table goes
//! in the container's key/value data, so an atlas is still one file.
//!
//! Only for UI art: clamp to edge, no tiling. The atlas carries a short mip
//! chain, so a sprite drawn well below native size does not alias.
//! Model textures cannot be atlased, because they repeat.

use anyhow::{Context, Result, bail};
use std::path::{Path, PathBuf};
use yeschef_format::ktx2::{self, Format};

use crate::image::{self, Rgba};
use crate::mip::{self, MipSpace};
use crate::pack;

const CHANNELS: usize = 4;

/// Mip levels written, the full size one included. Sprites drawn down to a
/// quarter of their size stay clean.
const LEVELS: u32 = 3;

/// Cells sit on multiples of this, so every texel of the last mip level lies
/// inside one cell and averages only that sprite and its own gutter.
const ALIGN: u32 = 1 << (LEVELS - 1);

/// Each sprite is surrounded by a border holding a copy of its edge pixels. A
/// linear sample at the very edge of a sprite then reads that copy instead of
/// the neighboring sprite. The mip filter reaches four texels, so the border
/// is wide enough that no level reads past it.
const GUTTER: u32 = 8;

/// The atlas is as small as the packer can make it, up to this side. A set of
/// sprites that does not fit belongs in its own texture, not in an atlas.
const MAX_SIDE: u32 = 2048;

/// Copies a sprite into its `cell` (width, height) at `(x, y)`, where the
/// sprite starts `GUTTER` texels in, and extends its edge pixels over the rest
/// of the cell.
fn blit(atlas: &mut [u8], atlas_width: u32, source: &Rgba, x: u32, y: u32, cell: (u32, u32)) {
    let (width, height) = (source.width as usize, source.height as usize);
    for row in 0..cell.1 as usize {
        let source_row = row.saturating_sub(GUTTER as usize).min(height - 1);
        for col in 0..cell.0 as usize {
            let source_col = col.saturating_sub(GUTTER as usize).min(width - 1);
            let from = (source_row * width + source_col) * CHANNELS;
            let to = ((y as usize + row) * atlas_width as usize + x as usize + col) * CHANNELS;
            atlas[to..to + CHANNELS].copy_from_slice(&source.pixels[from..from + CHANNELS]);
        }
    }
}

/// `name x y w h` per line. The table is text so it can be read in a hex dump
/// of the container, and it is short.
fn rect_table(names: &[&str], sizes: &[(u32, u32)], places: &[pack::Placement]) -> Vec<u8> {
    let mut table = String::new();
    for ((name, &(w, h)), place) in names.iter().zip(sizes).zip(places) {
        table += &format!(
            "{name} {} {} {w} {h}\n",
            place.x * ALIGN + GUTTER,
            place.y * ALIGN + GUTTER
        );
    }
    table.into_bytes()
}

/// Packs the sprites, given as a name and a source image each, into a KTX2
/// file.
pub fn pack_atlas(sprites: &[(&str, Rgba)]) -> Result<(Vec<u8>, u32, u32)> {
    // Packed in units of ALIGN, so every cell lands on the grid.
    let cells: Vec<(u32, u32)> = sprites
        .iter()
        .map(|(_, image)| {
            (
                (image.width + GUTTER * 2).div_ceil(ALIGN),
                (image.height + GUTTER * 2).div_ceil(ALIGN),
            )
        })
        .collect();
    let Some(((units_wide, units_high), places)) = pack::pack(&cells, MAX_SIDE / ALIGN) else {
        bail!(
            "{} sprites do not fit in a {MAX_SIDE}x{MAX_SIDE} atlas",
            sprites.len()
        );
    };
    let (width, height) = (units_wide * ALIGN, units_high * ALIGN);

    let mut pixels = vec![0u8; width as usize * height as usize * CHANNELS];
    for (((_, image), place), cell) in sprites.iter().zip(&places).zip(&cells) {
        blit(
            &mut pixels,
            width,
            image,
            place.x * ALIGN,
            place.y * ALIGN,
            (cell.0 * ALIGN, cell.1 * ALIGN),
        );
    }

    let mut levels = vec![pixels];
    for level in 1..LEVELS {
        let previous = &levels[level as usize - 1];
        levels.push(mip::halve_mip(
            previous,
            width >> (level - 1),
            height >> (level - 1),
            MipSpace::Srgb,
        ));
    }

    let names: Vec<&str> = sprites.iter().map(|(name, _)| *name).collect();
    let sizes: Vec<(u32, u32)> = sprites
        .iter()
        .map(|(_, image)| (image.width, image.height))
        .collect();
    // UNORM rather than sRGB, which is what the engine uploaded for these
    // images before they were baked.
    let key_values = [(
        "spongeAtlas".to_string(),
        rect_table(&names, &sizes, &places),
    )];
    let file = ktx2::write(Format::R8G8B8A8Unorm, width, height, &levels, &key_values);
    Ok((file, width, height))
}

pub fn convert(sprites: &[(String, PathBuf)], to: &Path) -> Result<()> {
    let mut images = Vec::with_capacity(sprites.len());
    for (name, path) in sprites {
        let bytes =
            std::fs::read(path).with_context(|| format!("unable to read {}", path.display()))?;
        let decoded = image::decode(&bytes)
            .with_context(|| format!("unable to decode {}", path.display()))?;
        if decoded.width == 0 || decoded.height == 0 {
            bail!("{} is empty", path.display());
        }
        images.push((name.as_str(), decoded));
    }
    let (file, width, height) = pack_atlas(&images)?;
    std::fs::write(to, &file).with_context(|| format!("unable to write {}", to.display()))?;
    crate::progress::say!(
        "{} sprites -> {} ({width}x{height}, {} bytes)",
        sprites.len(),
        crate::progress::display(to),
        file.len()
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sprite(width: u32, height: u32, texel: [u8; 4]) -> Rgba {
        Rgba {
            width,
            height,
            pixels: texel.repeat((width * height) as usize),
        }
    }

    #[test]
    fn sprites_land_where_the_table_says() {
        let sprites = [
            ("red", sprite(3, 2, [255, 0, 0, 255])),
            ("green", sprite(2, 4, [0, 255, 0, 255])),
        ];
        let (file, width, height) = pack_atlas(&sprites).unwrap();
        let image = ktx2::read(&file).unwrap();
        assert_eq!((image.width, image.height), (width, height));
        assert_eq!(image.vk_format, 37);
        assert_eq!(image.levels.len(), LEVELS as usize);

        let (key, table) = &image.key_values[0];
        assert_eq!(key, "spongeAtlas");
        let table = String::from_utf8(table.to_vec()).unwrap();
        let lines: Vec<Vec<&str>> = table.lines().map(|l| l.split(' ').collect()).collect();
        assert_eq!(lines.len(), 2);
        for (line, (name, sprite)) in lines.iter().zip(&sprites) {
            assert_eq!(line[0], *name);
            let (x, y): (usize, usize) = (line[1].parse().unwrap(), line[2].parse().unwrap());
            assert_eq!(line[3], sprite.width.to_string());
            assert_eq!(line[4], sprite.height.to_string());
            // Every texel of the sprite, and the gutter around it, has the
            // sprite's color.
            for row in y - 1..y + sprite.height as usize + 1 {
                for col in x - 1..x + sprite.width as usize + 1 {
                    let at = (row * width as usize + col) * CHANNELS;
                    assert_eq!(&image.levels[0][at..at + 4], &sprite.pixels[..4], "{name}");
                }
            }
        }
    }

    #[test]
    fn the_gutter_copies_the_edge_texels() {
        // A 2x1 sprite, red then blue. The gutter left of it is red and the
        // gutter right of it is blue.
        let mut image = sprite(2, 1, [255, 0, 0, 255]);
        image.pixels[4..].copy_from_slice(&[0, 0, 255, 255]);
        let (file, width, _) = pack_atlas(&[("a", image)]).unwrap();
        let level = ktx2::read(&file).unwrap().levels[0].to_vec();
        let texel = |x: usize, y: usize| level[(y * width as usize + x) * 4..][..4].to_vec();
        assert_eq!(texel(0, 0), [255, 0, 0, 255]);
        let right = GUTTER as usize + 2;
        assert_eq!(texel(right, 0), [0, 0, 255, 255]);
        assert_eq!(texel(right + GUTTER as usize - 1, 0), [0, 0, 255, 255]);
    }

    #[test]
    fn sprites_that_cannot_fit_are_an_error() {
        let huge = sprite(MAX_SIDE, 1, [0; 4]);
        assert!(pack_atlas(&[("big", huge)]).is_err());
    }

    #[test]
    fn mips_do_not_mix_neighboring_sprites() {
        let sprites = [
            ("red", sprite(10, 10, [255, 0, 0, 255])),
            ("green", sprite(10, 10, [0, 255, 0, 255])),
            ("blue", sprite(10, 10, [0, 0, 255, 255])),
        ];
        let (file, width, _) = pack_atlas(&sprites).unwrap();
        let image = ktx2::read(&file).unwrap();
        let table = String::from_utf8(image.key_values[0].1.to_vec()).unwrap();
        let level = LEVELS as usize - 1;
        let side = (width >> level) as usize;
        let scale = ALIGN as usize;
        for (line, (_, sprite)) in table.lines().zip(&sprites) {
            let f: Vec<usize> = line
                .split(' ')
                .skip(1)
                .map(|v| v.parse().unwrap())
                .collect();
            // Every texel a linear sample of this sprite can read: its rect,
            // widened by one texel of the last level.
            for row in f[1] / scale - 1..(f[1] + f[3]).div_ceil(scale) + 1 {
                for col in f[0] / scale - 1..(f[0] + f[2]).div_ceil(scale) + 1 {
                    let at = (row * side + col) * CHANNELS;
                    for c in 0..CHANNELS {
                        let got = image.levels[level][at + c];
                        assert!(got.abs_diff(sprite.pixels[c]) <= 1, "texel {col},{row}");
                    }
                }
            }
        }
    }
}
