//! Writing and reading KTX2 containers, the format of the baked textures,
//! fonts and environment maps.
//!
//! KTX2 is a standard format, so it stays out of cbindgen. The C++ reader in
//! `sponge/src/scene/format/ktx2.cpp` reads what this writes. The data format
//! descriptor is left empty because that reader needs `vkFormat` alone. Add a
//! descriptor if these files ever leave the build for outside KTX tools.

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

/// A key and its value, in the container's own place for metadata.
pub type KeyValue = (String, Vec<u8>);

fn pad_to_4(size: usize) -> usize {
    size.div_ceil(4) * 4
}

/// Wraps encoded level data in a KTX2 container. `levels[0]` is the full size
/// image. The layout is the header, the level index, the key/value data, then
/// the levels largest first. Offsets count from the start of the file.
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
    let key_value_offset = if key_value_size == 0 {
        0
    } else {
        HEADER + LEVEL_ENTRY * levels.len()
    };

    let mut cursor = HEADER + LEVEL_ENTRY * levels.len() + key_value_size;
    let mut out = Vec::with_capacity(cursor + levels.iter().map(Vec::len).sum::<usize>());
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

    put32(&mut out, 0); // dfdByteOffset
    put32(&mut out, 0); // dfdByteLength
    put32(&mut out, key_value_offset);
    put32(&mut out, key_value_size);
    put64(&mut out, 0); // sgdByteOffset
    put64(&mut out, 0); // sgdByteLength

    for level in levels {
        put64(&mut out, cursor);
        put64(&mut out, level.len());
        put64(&mut out, level.len()); // uncompressedByteLength
        cursor += level.len();
    }

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

    for level in levels {
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
        // The only level starts right after the one level entry.
        assert_eq!(u64::from_le_bytes(bytes[80..88].try_into().unwrap()), 104);
        assert_eq!(bytes.len(), 104 + 16);
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
