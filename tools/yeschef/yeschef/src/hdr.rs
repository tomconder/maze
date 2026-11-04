//! Radiance `.hdr` (RGBE) environment images, baked to half float KTX2.
//!
//! The image is not block compressed, because a block codec cannot hold HDR
//! radiance and a probe reads one level only.

use anyhow::{Context, Result, bail};
use std::path::Path;
use yeschef_format::ktx2::{self, Format};

const RGBE_BYTES: usize = 4;
const HALF_MAX: f32 = 65504.0;
const MIN_RLE_WIDTH: usize = 8;
const MAX_RLE_WIDTH: usize = 32767;

/// Float to IEEE half, rounding toward zero. Radiance values are positive and
/// the image is a light source, so the lost low bit does not matter. Values
/// past the half range clamp to its maximum.
fn to_half(value: f32) -> u16 {
    let bits = value.clamp(0.0, HALF_MAX).to_bits();
    let exponent = ((bits >> 23) & 0xFF) as i32 - 127 + 15;
    if exponent <= 0 {
        return 0; // below the smallest half: a black texel is close enough
    }
    ((exponent << 10) | ((bits >> 13) & 0x3FF) as i32) as u16
}

/// One header line without its newline, or `None` at the end of the file.
fn read_line<'a>(bytes: &'a [u8], cursor: &mut usize) -> Option<&'a [u8]> {
    if *cursor >= bytes.len() {
        return None;
    }
    let start = *cursor;
    while *cursor < bytes.len() && bytes[*cursor] != b'\n' {
        *cursor += 1;
    }
    let line = &bytes[start..*cursor];
    *cursor += 1;
    Some(line)
}

/// One scanline in the new run-length format: the four bytes of each pixel
/// are stored as four separate runs. Returns `None` if the data is bad.
fn read_rle_scanline(bytes: &[u8], cursor: &mut usize, scanline: &mut [u8]) -> Option<()> {
    let width = scanline.len() / RGBE_BYTES;
    for channel in 0..RGBE_BYTES {
        let mut x = 0;
        while x < width {
            let count = *bytes.get(*cursor)? as usize;
            *cursor += 1;
            if count > 128 {
                let count = count - 128;
                if x + count > width {
                    return None;
                }
                let value = *bytes.get(*cursor)?;
                *cursor += 1;
                for _ in 0..count {
                    scanline[x * RGBE_BYTES + channel] = value;
                    x += 1;
                }
            } else {
                if count == 0 || x + count > width {
                    return None;
                }
                let run = bytes.get(*cursor..*cursor + count)?;
                *cursor += count;
                for &value in run {
                    scanline[x * RGBE_BYTES + channel] = value;
                    x += 1;
                }
            }
        }
    }
    Some(())
}

/// Reads a Radiance image and returns it as a KTX2 file of half float RGBA
/// with one level.
pub fn bake(bytes: &[u8]) -> Result<(Vec<u8>, u32, u32)> {
    let mut cursor = 0;
    if !read_line(bytes, &mut cursor).is_some_and(|line| line.starts_with(b"#?")) {
        bail!("not a Radiance image");
    }
    // The header ends at a blank line, and the resolution follows.
    let mut rgbe = false;
    while let Some(line) = read_line(bytes, &mut cursor) {
        if line.is_empty() {
            break;
        }
        rgbe = rgbe || line == b"FORMAT=32-bit_rle_rgbe";
    }
    // Only the usual top-down, left-to-right order.
    let resolution = read_line(bytes, &mut cursor).unwrap_or_default();
    let resolution = String::from_utf8_lossy(resolution);
    let mut words = resolution.split_whitespace();
    let mut next = || words.next();
    let size = match (next(), next(), next(), next()) {
        (Some("-Y"), Some(height), Some("+X"), Some(width)) => {
            height.parse::<u32>().ok().zip(width.parse::<u32>().ok())
        }
        _ => None,
    };
    let Some((height, width)) = size.filter(|&(h, w)| rgbe && h > 0 && w > 0) else {
        bail!("needs 32-bit_rle_rgbe and a '-Y h +X w' resolution line");
    };
    let (width_px, height_px) = (width as usize, height as usize);

    let mut rgbe_line = vec![0u8; width_px * RGBE_BYTES];
    let mut out = Vec::with_capacity(width_px * height_px * 4 * 2);
    let one = to_half(1.0);

    for y in 0..height_px {
        let rest = &bytes[cursor.min(bytes.len())..];
        // Scanlines start with 2, 2 and the width, in a width this format can
        // run-length encode. Anything else is flat pixels.
        let rle = rest.len() >= RGBE_BYTES
            && (MIN_RLE_WIDTH..=MAX_RLE_WIDTH).contains(&width_px)
            && rest[0] == 2
            && rest[1] == 2
            && ((usize::from(rest[2]) << 8) | usize::from(rest[3])) == width_px;
        let truncated = || format!("truncated at row {y}");
        if rle {
            cursor += RGBE_BYTES;
            read_rle_scanline(bytes, &mut cursor, &mut rgbe_line).with_context(truncated)?;
        } else {
            let flat = rest.get(..rgbe_line.len()).with_context(truncated)?;
            rgbe_line.copy_from_slice(flat);
            cursor += rgbe_line.len();
        }

        for p in rgbe_line.as_chunks::<RGBE_BYTES>().0 {
            // value = mantissa / 256 * 2^(e - 128), and e == 0 is black. The
            // power of two is exact in a float.
            let scale = if p[3] == 0 {
                0.0
            } else {
                2f64.powi(i32::from(p[3]) - 136) as f32
            };
            for &channel in &p[..3] {
                out.extend_from_slice(&to_half(f32::from(channel) * scale).to_le_bytes());
            }
            out.extend_from_slice(&one.to_le_bytes());
        }
    }

    let file = ktx2::write(Format::R16G16B16A16Sfloat, width, height, &[out], &[]);
    Ok((file, width, height))
}

pub fn convert(from: &Path, to: &Path) -> Result<()> {
    let bytes =
        std::fs::read(from).with_context(|| format!("unable to read {}", from.display()))?;
    if bytes.is_empty() {
        bail!("unable to read {}", from.display());
    }
    let (file, width, height) = bake(&bytes).with_context(|| from.display().to_string())?;
    std::fs::write(to, file).with_context(|| format!("unable to write {}", to.display()))?;
    println!(
        "{} ({width}x{height} RGBE) -> half float",
        from.to_string_lossy().replace('\\', "/")
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    const HEADER: &[u8] = b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n";

    fn image(resolution: &str, pixels: &[u8]) -> Vec<u8> {
        [HEADER, resolution.as_bytes(), b"\n", pixels].concat()
    }

    fn texels(file: &[u8]) -> Vec<u16> {
        let image = ktx2::read(file).unwrap();
        image.levels[0]
            .as_chunks::<2>()
            .0
            .iter()
            .map(|b| u16::from_le_bytes([b[0], b[1]]))
            .collect()
    }

    #[test]
    fn halves_round_toward_zero_and_clamp() {
        assert_eq!(to_half(1.0), 0x3C00);
        assert_eq!(to_half(0.5), 0x3800);
        assert_eq!(to_half(0.0), 0);
        assert_eq!(to_half(1e-9), 0, "below the smallest half is black");
        assert_eq!(to_half(1e9), 0x7BFF, "past the range clamps to the maximum");
        assert_eq!(
            to_half(1.0 + 1.0 / 2048.0),
            0x3C00,
            "the low bit is dropped"
        );
    }

    #[test]
    fn flat_pixels_decode() {
        // 128 / 256 * 2^(129 - 128) = 1.0; exponent 0 is black.
        let file = bake(&image("-Y 1 +X 2", &[128, 64, 0, 129, 255, 255, 255, 0])).unwrap();
        assert_eq!((file.1, file.2), (2, 1));
        assert_eq!(
            texels(&file.0),
            [0x3C00, 0x3800, 0, 0x3C00, 0, 0, 0, 0x3C00]
        );
    }

    #[test]
    fn run_length_scanlines_decode() {
        // Width 8: 2, 2, 0, 8, then each channel as one run of 8 (128 + 8).
        let mut pixels = vec![2, 2, 0, 8];
        for value in [128u8, 128, 128, 129] {
            pixels.extend_from_slice(&[136, value]);
        }
        let file = bake(&image("-Y 1 +X 8", &pixels)).unwrap();
        let texels = texels(&file.0);
        assert_eq!(texels.len(), 32);
        assert!(
            texels
                .chunks(4)
                .all(|t| t == [0x3C00, 0x3C00, 0x3C00, 0x3C00])
        );
    }

    #[test]
    fn bad_images_are_errors() {
        for (name, bytes) in [
            ("not radiance", b"P6\n1 1\n255\n".to_vec()),
            ("no format", b"#?RADIANCE\n\n-Y 1 +X 1\n\0\0\0\0".to_vec()),
            ("bottom up", image("+Y 1 +X 1", &[0; 4])),
            ("zero width", image("-Y 1 +X 0", &[])),
            ("truncated", image("-Y 2 +X 2", &[0; 8])),
            ("bad run", image("-Y 1 +X 8", &[2, 2, 0, 8, 0])),
        ] {
            assert!(bake(&bytes).is_err(), "{name}");
        }
    }
}
