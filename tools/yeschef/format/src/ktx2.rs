//! Writing and reading KTX2 containers, the format of the baked textures,
//! fonts and environment maps.
//!
//! KTX2 is a standard format, so it stays out of cbindgen. The C++ reader in
//! `sponge/src/scene/format/ktx2.cpp` reads what this writes. That reader needs
//! `vkFormat` alone. The data format descriptor and the level layout are there
//! for outside KTX tools. Check them with `ktx validate`.

const IDENTIFIER: [u8; 12] = [
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A,
];

/// The fixed part of the file: the identifier, nine `u32` fields, then the
/// index of four `u32` and two `u64`. The level index follows it.
const HEADER: usize = 80;
const LEVEL_ENTRY: usize = 24;

/// The values KTX2 stores in its `vkFormat` field, a subset of `VkFormat`.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum Format {
    R8G8B8Unorm = 23,
    R8G8B8A8Unorm = 37,
    /// Half float, for HDR images such as an environment map.
    R16G16B16A16Sfloat = 97,
    Bc5Unorm = 141,
    Bc7Unorm = 145,
}

impl Format {
    /// Bytes in one texel, or in one 4x4 block of a compressed format.
    fn block_size(self) -> usize {
        match self {
            Format::R8G8B8Unorm => 3,
            Format::R8G8B8A8Unorm => 4,
            Format::R16G16B16A16Sfloat => 8,
            Format::Bc5Unorm | Format::Bc7Unorm => 16,
        }
    }
}

/// The data format descriptor: its total size, then one basic descriptor block
/// as the Khronos Data Format Specification lays it out. Primaries are BT.709
/// and the transfer function is linear, as every format here is UNORM or float.
fn descriptor(format: Format) -> Vec<u8> {
    // Color models.
    const RGBSDA: u32 = 1;
    const BC5: u32 = 132;
    const BC7: u32 = 134;
    // Channel ids. Alpha is 15 in the RGBSDA model.
    const RGBA: [u32; 4] = [0, 1, 2, 15];
    // Qualifiers of a signed float channel.
    const SIGNED_FLOAT: u32 = 0xC0;

    // A sample is (bit offset, bit length, channel, lower, upper).
    let channels = |count: usize, bits: u32, flags: u32, lower: u32, upper: u32| {
        (0..count)
            .map(|i| (i as u32 * bits, bits, RGBA[i] | flags, lower, upper))
            .collect::<Vec<_>>()
    };
    let (model, block_side, samples) = match format {
        Format::R8G8B8Unorm => (RGBSDA, 1, channels(3, 8, 0, 0, 255)),
        Format::R8G8B8A8Unorm => (RGBSDA, 1, channels(4, 8, 0, 0, 255)),
        Format::R16G16B16A16Sfloat => (
            RGBSDA,
            1,
            channels(4, 16, SIGNED_FLOAT, (-1.0f32).to_bits(), 1.0f32.to_bits()),
        ),
        Format::Bc5Unorm => (BC5, 4, channels(2, 64, 0, 0, u32::MAX)),
        Format::Bc7Unorm => (BC7, 4, channels(1, 128, 0, 0, u32::MAX)),
    };

    let block_bytes = 24 + 16 * samples.len() as u32;
    let mut words = vec![
        4 + block_bytes,
        0,                       // Khronos vendor, basic descriptor type
        2 | (block_bytes << 16), // version 1.3
        model | (1 << 8) | (1 << 16),
        (block_side - 1) | ((block_side - 1) << 8),
        format.block_size() as u32, // bytes in plane 0
        0,
    ];
    for (offset, bits, channel, lower, upper) in samples {
        words.extend([
            offset | ((bits - 1) << 16) | (channel << 24),
            0,
            lower,
            upper,
        ]);
    }
    words.iter().flat_map(|word| word.to_le_bytes()).collect()
}

/// A key and its value, in the container's own place for metadata.
pub type KeyValue = (String, Vec<u8>);

fn pad_to_4(size: usize) -> usize {
    size.div_ceil(4) * 4
}

/// Wraps encoded level data in a KTX2 container. `levels[0]` is the full size
/// image. The layout is the header, the level index, the data format
/// descriptor, the key/value data, then the levels smallest first, as KTX2
/// requires. Offsets count from the start of the file.
pub fn write(
    format: Format,
    width: u32,
    height: u32,
    levels: &[Vec<u8>],
    key_values: &[KeyValue],
) -> Vec<u8> {
    let key_value_size: usize = key_values
        .iter()
        .map(|(key, value)| pad_to_4(4 + key.len() + 1 + value.len()))
        .sum();
    let descriptor = descriptor(format);
    let descriptor_offset = HEADER + LEVEL_ENTRY * levels.len();
    let key_value_offset = if key_value_size == 0 {
        0
    } else {
        descriptor_offset + descriptor.len()
    };

    // Each level starts on a multiple of both the block size and 4.
    let alignment = match format.block_size() {
        3 => 12,
        size => size.max(4),
    };
    let mut cursor = descriptor_offset + descriptor.len() + key_value_size;
    let mut offsets = vec![0; levels.len()];
    for (offset, level) in offsets.iter_mut().zip(levels).rev() {
        *offset = cursor.next_multiple_of(alignment);
        cursor = *offset + level.len();
    }
    let mut out = Vec::with_capacity(cursor);
    out.extend_from_slice(&IDENTIFIER);

    let put32 = |out: &mut Vec<u8>, value: usize| {
        out.extend_from_slice(&u32::try_from(value).expect("fits in 32 bits").to_le_bytes());
    };
    let put64 = |out: &mut Vec<u8>, value: usize| {
        out.extend_from_slice(&(value as u64).to_le_bytes());
    };

    put32(&mut out, format as usize);
    // The size of one channel for an uncompressed format, and 1 by definition
    // for a block compressed one.
    put32(
        &mut out,
        if format == Format::R16G16B16A16Sfloat {
            2
        } else {
            1
        },
    );
    put32(&mut out, width as usize);
    put32(&mut out, height as usize);
    put32(&mut out, 0); // pixelDepth
    put32(&mut out, 0); // layerCount
    put32(&mut out, 1); // faceCount
    put32(&mut out, levels.len());
    put32(&mut out, 0); // supercompressionScheme: none

    put32(&mut out, descriptor_offset);
    put32(&mut out, descriptor.len());
    put32(&mut out, key_value_offset);
    put32(&mut out, key_value_size);
    put64(&mut out, 0); // sgdByteOffset
    put64(&mut out, 0); // sgdByteLength

    for (level, &offset) in levels.iter().zip(&offsets) {
        put64(&mut out, offset);
        put64(&mut out, level.len());
        put64(&mut out, level.len()); // uncompressedByteLength
    }
    out.extend_from_slice(&descriptor);

    // Each entry is a length, a NUL terminated key, the value, then padding to
    // the next 4 bytes.
    for (key, value) in key_values {
        let entry = 4 + key.len() + 1 + value.len();
        put32(&mut out, key.len() + 1 + value.len());
        out.extend_from_slice(key.as_bytes());
        out.push(0);
        out.extend_from_slice(value);
        out.resize(out.len() + pad_to_4(entry) - entry, 0);
    }

    for (level, &offset) in levels.iter().zip(&offsets).rev() {
        out.resize(offset, 0);
        out.extend_from_slice(level);
    }
    out
}

/// What the tests and `--verify` need from a file: the format fields, the
/// level bytes and the key/value pairs.
#[derive(Debug, PartialEq, Eq)]
pub struct Image<'a> {
    pub vk_format: u32,
    pub width: u32,
    pub height: u32,
    pub levels: Vec<&'a [u8]>,
    pub key_values: Vec<(String, &'a [u8])>,
}

pub fn read(bytes: &[u8]) -> Result<Image<'_>, String> {
    let word = |at: usize| -> Result<u32, String> {
        bytes
            .get(at..at + 4)
            .map(|b| u32::from_le_bytes(b.try_into().unwrap()))
            .ok_or_else(|| "the KTX2 file is truncated".to_string())
    };
    let long = |at: usize| -> Result<usize, String> {
        bytes
            .get(at..at + 8)
            .map(|b| u64::from_le_bytes(b.try_into().unwrap()) as usize)
            .ok_or_else(|| "the KTX2 file is truncated".to_string())
    };
    if bytes.len() < HEADER || bytes[..12] != IDENTIFIER {
        return Err("not a KTX2 file".into());
    }
    let supercompression = word(44)?;
    if supercompression != 0 {
        return Err(format!(
            "KTX2 supercompression scheme {supercompression} is not supported"
        ));
    }

    let level_count = word(40)?.max(1) as usize;
    let mut levels = Vec::with_capacity(level_count);
    for i in 0..level_count {
        let entry = HEADER + LEVEL_ENTRY * i;
        let (offset, length) = (long(entry)?, long(entry + 8)?);
        levels.push(
            bytes
                .get(offset..offset + length)
                .ok_or(format!("KTX2 level {i} runs past the end of the file"))?,
        );
    }

    let (kv_offset, kv_length) = (word(56)? as usize, word(60)? as usize);
    let kv = bytes
        .get(kv_offset..kv_offset + kv_length)
        .ok_or("KTX2 key/value data runs past the end of the file")?;
    let mut key_values = Vec::new();
    let mut at = 0;
    while at + 4 <= kv.len() {
        let length = u32::from_le_bytes(kv[at..at + 4].try_into().unwrap()) as usize;
        let entry = kv
            .get(at + 4..at + 4 + length)
            .ok_or("KTX2 key/value entry runs past its data")?;
        let nul = entry
            .iter()
            .position(|&b| b == 0)
            .ok_or("KTX2 key/value entry has no key terminator")?;
        key_values.push((
            String::from_utf8_lossy(&entry[..nul]).into_owned(),
            &entry[nul + 1..],
        ));
        at += pad_to_4(4 + length);
    }

    Ok(Image {
        vk_format: word(12)?,
        width: word(20)?,
        height: word(24)?,
        levels,
        key_values,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_file_reads_back() {
        let levels = vec![vec![1u8; 16], vec![2u8; 4]];
        let pairs = vec![("key".to_string(), vec![9u8, 8, 7])];
        let bytes = write(Format::Bc7Unorm, 4, 4, &levels, &pairs);
        let image = read(&bytes).unwrap();
        assert_eq!(image.vk_format, 145);
        assert_eq!((image.width, image.height), (4, 4));
        assert_eq!(image.levels, [&levels[0][..], &levels[1][..]]);
        assert_eq!(image.key_values, [("key".to_string(), &[9u8, 8, 7][..])]);
    }

    #[test]
    fn the_header_fields_are_where_the_cpp_reader_looks() {
        let bytes = write(Format::R16G16B16A16Sfloat, 2, 1, &[vec![0; 16]], &[]);
        assert_eq!(&bytes[..12], &IDENTIFIER);
        assert_eq!(u32::from_le_bytes(bytes[12..16].try_into().unwrap()), 97);
        assert_eq!(u32::from_le_bytes(bytes[16..20].try_into().unwrap()), 2); // typeSize
        assert_eq!(u32::from_le_bytes(bytes[40..44].try_into().unwrap()), 1); // levelCount
        // The descriptor follows the one level entry: 28 bytes of sizes and
        // fields, then 4 samples of 16 bytes.
        assert_eq!(u32::from_le_bytes(bytes[48..52].try_into().unwrap()), 104);
        assert_eq!(u32::from_le_bytes(bytes[52..56].try_into().unwrap()), 92);
        assert_eq!(u32::from_le_bytes(bytes[104..108].try_into().unwrap()), 92);
        // The level starts at the next multiple of its 8 byte texel.
        assert_eq!(u64::from_le_bytes(bytes[80..88].try_into().unwrap()), 200);
        assert_eq!(bytes.len(), 200 + 16);
    }

    #[test]
    fn levels_are_aligned_and_stored_smallest_first() {
        let levels = vec![vec![1u8; 48], vec![2u8; 12], vec![3u8; 3]];
        let pairs = vec![("k".to_string(), vec![9u8])];
        let bytes = write(Format::R8G8B8Unorm, 4, 4, &levels, &pairs);
        let word = |at: usize| u32::from_le_bytes(bytes[at..at + 4].try_into().unwrap()) as usize;
        // The key/value data follows the descriptor.
        assert_eq!(word(56), word(48) + word(52));
        let offsets: Vec<usize> = (0..3).map(|i| word(80 + 24 * i)).collect();
        assert!(offsets[2] < offsets[1] && offsets[1] < offsets[0]);
        assert!(offsets.iter().all(|offset| offset % 12 == 0));
        assert_eq!(read(&bytes).unwrap().levels, levels);
    }

    #[test]
    fn a_bad_file_is_an_error_not_a_panic() {
        let good = write(Format::Bc7Unorm, 4, 4, &[vec![0; 16]], &[]);
        assert!(read(&good[..40]).is_err());
        assert!(read(&good[..good.len() - 1]).is_err());
        let mut bad = good;
        bad[0] = 0;
        assert!(read(&bad).is_err());
    }
}
