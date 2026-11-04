//! Packs several small images into one KTX2 sprite sheet. The rect table goes
//! in the container's key/value data, so an atlas is still one file.
//!
//! Only for UI art: clamp to edge, no tiling, drawn at or near native size.
//! Model textures cannot be atlased, because they repeat.

use anyhow::{Context, Result, bail};
use std::path::{Path, PathBuf};
use yeschef_format::ktx2::{self, Format};

use crate::image::{self, Rgba};
use crate::pack;

const CHANNELS: usize = 4;

/// Each sprite is surrounded by a one texel border holding a copy of its edge
/// pixels. A linear sample at the very edge of a sprite then reads that copy
/// instead of the neighbouring sprite.
const GUTTER: u32 = 1;

/// The atlas is as small as the packer can make it, up to this side. A set of
/// sprites that does not fit belongs in its own texture, not in an atlas.
const MAX_SIDE: u32 = 2048;

/// Copies a sprite in at `(x, y)` and extends its edge pixels into the gutter.
fn blit(atlas: &mut [u8], atlas_width: u32, source: &Rgba, x: u32, y: u32) {
    let (width, height) = (source.width as i64, source.height as i64);
    let gutter = i64::from(GUTTER);
    for row in -gutter..height + gutter {
        let source_row = row.clamp(0, height - 1) as usize;
        for col in -gutter..width + gutter {
            let source_col = col.clamp(0, width - 1) as usize;
            let from = (source_row * source.width as usize + source_col) * CHANNELS;
            let to = ((i64::from(y) + row) as usize * atlas_width as usize
                + (i64::from(x) + col) as usize)
                * CHANNELS;
            atlas[to..to + CHANNELS].copy_from_slice(&source.pixels[from..from + CHANNELS]);
        }
    }
}

/// `name x y w h` per line. The table is text so it can be read in a hex dump
/// of the container, and it is short.
fn rect_table(names: &[&str], sizes: &[(u32, u32)], places: &[pack::Placement]) -> Vec<u8> {
    let mut table = String::new();
    for ((name, &(w, h)), place) in names.iter().zip(sizes).zip(places) {
        table += &format!("{name} {} {} {w} {h}\n", place.x + GUTTER, place.y + GUTTER);
    }
    table.into_bytes()
}

/// Packs the sprites, given as a name and a source image each, into a KTX2
/// file.
pub fn pack_atlas(sprites: &[(&str, Rgba)]) -> Result<(Vec<u8>, u32, u32)> {
    let padded: Vec<(u32, u32)> = sprites
        .iter()
        .map(|(_, image)| (image.width + GUTTER * 2, image.height + GUTTER * 2))
        .collect();
    let Some(((width, height), places)) = pack::pack(&padded, MAX_SIDE) else {
        bail!(
            "{} sprites do not fit in a {MAX_SIDE}x{MAX_SIDE} atlas",
            sprites.len()
        );
    };

    let mut pixels = vec![0u8; width as usize * height as usize * CHANNELS];
    for ((_, image), place) in sprites.iter().zip(&places) {
        blit(
            &mut pixels,
            width,
            image,
            place.x + GUTTER,
            place.y + GUTTER,
        );
    }

    let names: Vec<&str> = sprites.iter().map(|(name, _)| *name).collect();
    let sizes: Vec<(u32, u32)> = sprites
        .iter()
        .map(|(_, image)| (image.width, image.height))
        .collect();
    // One level, and UNORM rather than sRGB, which is what the engine uploaded
    // for these images before they were baked. Mips would mix neighbouring
    // sprites anyway, and UI is drawn at native size.
    let key_values = [(
        "spongeAtlas".to_string(),
        rect_table(&names, &sizes, &places),
    )];
    let file = ktx2::write(Format::R8G8B8A8Unorm, width, height, &[pixels], &key_values);
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
    println!(
        "{} sprites -> {} ({width}x{height}, {} bytes)",
        sprites.len(),
        to.to_string_lossy().replace('\\', "/"),
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
        assert_eq!(image.levels.len(), 1);

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
        assert_eq!(texel(0, 1), [255, 0, 0, 255]);
        assert_eq!(texel(3, 1), [0, 0, 255, 255]);
    }

    #[test]
    fn sprites_that_cannot_fit_are_an_error() {
        let huge = sprite(MAX_SIDE, 1, [0; 4]);
        assert!(pack_atlas(&[("big", huge)]).is_err());
    }
}
