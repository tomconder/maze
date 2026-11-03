//! Reading and writing the shader pack, `shaders.spnga`.
//!
//! Layout: the header, then one entry per shader sorted by name, then each
//! name and source in that order. Offsets count from the start of the file.
//! Every field is little endian.

use crate::{SHADERPACK_MAGIC, SHADERPACK_VERSION, ShaderPackEntry, ShaderPackHeader};
use std::collections::BTreeMap;

const HEADER: usize = size_of::<ShaderPackHeader>();
const ENTRY: usize = size_of::<ShaderPackEntry>();

/// Shader name to GLSL source.
pub type Sources = BTreeMap<String, String>;

/// The bytes of the pack. Fails when the file would pass 4 GiB, which the
/// 32 bit offsets cannot address.
pub fn write(sources: &Sources) -> Result<Vec<u8>, String> {
    let mut out = Vec::with_capacity(HEADER + ENTRY * sources.len());
    out.extend_from_slice(&SHADERPACK_MAGIC.to_le_bytes());
    out.extend_from_slice(&SHADERPACK_VERSION.to_le_bytes());
    out.extend_from_slice(
        &u32::try_from(sources.len())
            .map_err(|_| "too many shaders")?
            .to_le_bytes(),
    );

    // The strings go after the entry table.
    let mut strings: Vec<u8> = Vec::new();
    let base = HEADER + ENTRY * sources.len();
    let mut append = |text: &str| -> Result<(u32, u32), String> {
        let offset =
            u32::try_from(base + strings.len()).map_err(|_| "the shader pack would pass 4 GiB")?;
        let size = u32::try_from(text.len()).map_err(|_| "a shader passes 4 GiB")?;
        strings.extend_from_slice(text.as_bytes());
        Ok((offset, size))
    };
    for (name, source) in sources {
        let (name_offset, name_size) = append(name)?;
        let (source_offset, source_size) = append(source)?;
        for field in [name_offset, name_size, source_offset, source_size] {
            out.extend_from_slice(&field.to_le_bytes());
        }
    }
    if base + strings.len() > u32::MAX as usize {
        return Err("the shader pack would pass 4 GiB".into());
    }
    out.extend_from_slice(&strings);
    Ok(out)
}

/// The shaders in a pack. Every offset is checked against the file size.
pub fn read(bytes: &[u8]) -> Result<Sources, String> {
    let word = |at: usize| -> Result<u32, String> {
        bytes
            .get(at..at + 4)
            .map(|b| u32::from_le_bytes(b.try_into().unwrap()))
            .ok_or_else(|| "the shader pack is truncated".to_string())
    };
    if bytes.len() < HEADER {
        return Err("the shader pack is truncated".into());
    }
    if bytes[..8] != SHADERPACK_MAGIC.to_le_bytes() {
        return Err("not a shader pack".into());
    }
    let version = word(8)?;
    if version != SHADERPACK_VERSION {
        return Err(format!(
            "shader pack version {version}, expected {SHADERPACK_VERSION}"
        ));
    }
    let count = word(12)? as usize;

    let text = |offset: u32, size: u32| -> Result<String, String> {
        let (offset, size) = (offset as usize, size as usize);
        let slice = bytes
            .get(offset..offset + size)
            .ok_or("a shader runs past the end of the shader pack")?;
        String::from_utf8(slice.to_vec()).map_err(|_| "a shader is not UTF-8".to_string())
    };
    let mut sources = Sources::new();
    for i in 0..count {
        let at = HEADER + ENTRY * i;
        let name = text(word(at)?, word(at + 4)?)?;
        let source = text(word(at + 8)?, word(at + 12)?)?;
        sources.insert(name, source);
    }
    Ok(sources)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sample() -> Sources {
        Sources::from([
            ("b.frag".to_string(), "void main() {}\n".to_string()),
            ("a.vert".to_string(), "#version 450\n".to_string()),
            ("empty.comp".to_string(), String::new()),
        ])
    }

    #[test]
    fn a_pack_reads_back() {
        let sources = sample();
        assert_eq!(read(&write(&sources).unwrap()).unwrap(), sources);
    }

    #[test]
    fn the_layout_is_the_documented_one() {
        let bytes = write(&sample()).unwrap();
        assert_eq!(&bytes[..8], b"SPNGSH\0\0");
        // Three entries sort a.vert, b.frag, empty.comp. The first name starts
        // after the header and the table.
        let first_name = u32::from_le_bytes(bytes[16..20].try_into().unwrap());
        assert_eq!(first_name as usize, HEADER + ENTRY * 3);
        assert_eq!(&bytes[first_name as usize..][..6], b"a.vert");
    }

    #[test]
    fn a_bad_pack_is_an_error_not_a_panic() {
        let good = write(&sample()).unwrap();
        assert!(read(&good[..10]).is_err());
        assert!(read(&good[..good.len() - 1]).is_err());
        let mut bad_magic = good.clone();
        bad_magic[0] = b'X';
        assert!(read(&bad_magic).is_err());
        let mut bad_version = good;
        bad_version[8] = 99;
        assert!(read(&bad_version).is_err());
    }
}
