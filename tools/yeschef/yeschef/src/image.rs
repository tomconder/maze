//! Decodes a PNG or JPEG to RGBA8, choosing the codec from the magic bytes.

use anyhow::{Context, Result, anyhow, bail};
use zune_core::bytestream::ZCursor;
use zune_core::colorspace::ColorSpace;
use zune_core::options::DecoderOptions;
use zune_jpeg::JpegDecoder;

const PNG_MAGIC: [u8; 4] = [0x89, b'P', b'N', b'G'];
const JPEG_MAGIC: [u8; 3] = [0xFF, 0xD8, 0xFF];

/// Pixels as 8 bit RGBA, top row first.
pub struct Rgba {
    pub width: u32,
    pub height: u32,
    pub pixels: Vec<u8>,
}

pub fn decode(bytes: &[u8]) -> Result<Rgba> {
    if bytes.starts_with(&PNG_MAGIC) {
        decode_png(bytes)
    } else if bytes.starts_with(&JPEG_MAGIC) {
        decode_jpeg(bytes)
    } else {
        bail!("not a PNG or JPEG")
    }
}

fn decode_png(bytes: &[u8]) -> Result<Rgba> {
    let mut decoder = png::Decoder::new(std::io::Cursor::new(bytes));
    // Palettes and low bit depths expand, 16 bit channels drop to 8, and tRNS
    // becomes alpha. Gamma is left alone.
    decoder.set_transformations(png::Transformations::normalize_to_color8());
    let mut reader = decoder.read_info()?;
    let size = reader
        .output_buffer_size()
        .context("the image is too large")?;
    let mut data = vec![0; size];
    let info = reader.next_frame(&mut data)?;
    data.truncate(info.buffer_size());

    let pixels = match info.color_type {
        png::ColorType::Rgba => data,
        png::ColorType::Rgb => data
            .as_chunks::<3>()
            .0
            .iter()
            .flat_map(|p| [p[0], p[1], p[2], 255])
            .collect(),
        png::ColorType::Grayscale => data.iter().flat_map(|&g| [g, g, g, 255]).collect(),
        png::ColorType::GrayscaleAlpha => data
            .as_chunks::<2>()
            .0
            .iter()
            .flat_map(|p| [p[0], p[0], p[0], p[1]])
            .collect(),
        png::ColorType::Indexed => bail!("a palette was not expanded"),
    };
    Ok(Rgba {
        width: info.width,
        height: info.height,
        pixels,
    })
}

fn decode_jpeg(bytes: &[u8]) -> Result<Rgba> {
    let options = DecoderOptions::default().jpeg_set_out_colorspace(ColorSpace::RGBA);
    let mut decoder = JpegDecoder::new_with_options(ZCursor::new(bytes), options);
    let pixels = decoder.decode().map_err(|error| anyhow!("{error:?}"))?;
    let info = decoder.info().context("the JPEG has no header")?;
    Ok(Rgba {
        width: u32::from(info.width),
        height: u32::from(info.height),
        pixels,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn png(
        color: png::ColorType,
        depth: png::BitDepth,
        data: &[u8],
        palette: Option<Vec<u8>>,
    ) -> Vec<u8> {
        let mut out = Vec::new();
        let mut encoder = png::Encoder::new(&mut out, 2, 1);
        encoder.set_color(color);
        encoder.set_depth(depth);
        if let Some(palette) = palette {
            encoder.set_palette(palette);
        }
        encoder
            .write_header()
            .unwrap()
            .write_image_data(data)
            .unwrap();
        out
    }

    #[test]
    fn png_color_types_become_rgba() {
        use png::{BitDepth::*, ColorType::*};
        let rgb = decode(&png(Rgb, Eight, &[1, 2, 3, 4, 5, 6], None)).unwrap();
        assert_eq!(rgb.pixels, [1, 2, 3, 255, 4, 5, 6, 255]);
        let rgba = decode(&png(Rgba, Eight, &[1, 2, 3, 4, 5, 6, 7, 8], None)).unwrap();
        assert_eq!(rgba.pixels, [1, 2, 3, 4, 5, 6, 7, 8]);
        let gray = decode(&png(Grayscale, Eight, &[9, 200], None)).unwrap();
        assert_eq!(gray.pixels, [9, 9, 9, 255, 200, 200, 200, 255]);
        let gray_alpha = decode(&png(GrayscaleAlpha, Eight, &[9, 1, 200, 2], None)).unwrap();
        assert_eq!(gray_alpha.pixels, [9, 9, 9, 1, 200, 200, 200, 2]);
        let indexed = decode(&png(
            Indexed,
            Eight,
            &[1, 0],
            Some(vec![10, 11, 12, 20, 21, 22]),
        ))
        .unwrap();
        assert_eq!(indexed.pixels, [20, 21, 22, 255, 10, 11, 12, 255]);
        assert_eq!((indexed.width, indexed.height), (2, 1));
    }

    #[test]
    fn sixteen_bit_channels_keep_the_high_byte() {
        use png::{BitDepth::Sixteen, ColorType::Grayscale};
        let image = decode(&png(Grayscale, Sixteen, &[0x12, 0xFF, 0xAB, 0x00], None)).unwrap();
        assert_eq!(image.pixels, [0x12, 0x12, 0x12, 255, 0xAB, 0xAB, 0xAB, 255]);
    }

    #[test]
    fn other_data_is_an_error() {
        assert!(decode(b"GIF89a").is_err());
        assert!(decode(&PNG_MAGIC).is_err());
        assert!(decode(&JPEG_MAGIC).is_err());
    }
}

/// Decodes every file in the folder named by `YESCHEF_DECODE_DIR` and writes
/// `<name>.rgba` beside it, to compare against another decoder. Run with
/// `cargo test dump_decoded -- --ignored`.
#[cfg(test)]
#[test]
#[ignore]
fn dump_decoded() {
    let dir = std::path::PathBuf::from(std::env::var("YESCHEF_DECODE_DIR").unwrap());
    for entry in std::fs::read_dir(&dir).unwrap() {
        let path = entry.unwrap().path();
        if path.extension().is_some_and(|e| e == "jpg" || e == "png") {
            let image = decode(&std::fs::read(&path).unwrap()).unwrap();
            let mut out = [image.width.to_le_bytes(), image.height.to_le_bytes()].concat();
            out.extend_from_slice(&image.pixels);
            std::fs::write(path.with_extension("rgba"), out).unwrap();
        }
    }
}
