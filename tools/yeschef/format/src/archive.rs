//! The asset pack: one index, `assets.spnidx`, and numbered data files.
//!
//! Index layout: the header, then the size of each data file as a `u64`, then
//! the entries, then the names. A data file starts with `ArchiveDataHeader`.
//! Each entry is one complete baked file. Names are relative to the asset root
//! and the entries are in the order of their name bytes. Every field is little
//! endian.

use bytemuck::{bytes_of, pod_read_unaligned};
use std::mem::size_of;

/// Asset pack version.
pub const ARCHIVE_VERSION: u32 = 1;

/// Index magic, the bytes `SPNGPK\0\0` read as a little endian integer.
pub const ARCHIVE_MAGIC: u64 = 0x0000_4B50_474E_5053;

/// Data file magic, the bytes `SPNGPD\0\0` read as a little endian integer.
pub const ARCHIVE_DATA_MAGIC: u64 = 0x0000_4450_474E_5053;

/// Each entry starts on a multiple of this many bytes in its data file.
pub const ARCHIVE_ALIGNMENT: u64 = 4096;

/// The only compression value that the readers accept.
pub const ARCHIVE_COMPRESSION_NONE: u32 = 0;

/// Start of the index.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct ArchiveHeader {
    /// The bytes of `ARCHIVE_MAGIC`.
    pub magic: [u8; 8],
    pub version: u32,
    pub entry_count: u32,
    pub data_file_count: u32,
    pub names_size: u32,
    /// The same value in the index and in each data file of one build.
    pub build_id: u64,
}

/// One asset in the index.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct ArchiveEntry {
    /// From the start of the names block.
    pub name_offset: u32,
    pub name_size: u32,
    pub data_file: u32,
    pub compression: u32,
    /// From the start of the data file.
    pub offset: u64,
    /// Stored bytes.
    pub size: u64,
    /// Equals `size` when `compression` is 0.
    pub raw_size: u64,
}

/// Start of a data file.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct ArchiveDataHeader {
    /// The bytes of `ARCHIVE_DATA_MAGIC`.
    pub magic: [u8; 8],
    pub version: u32,
    pub data_file: u32,
    pub build_id: u64,
}

const HEADER: usize = size_of::<ArchiveHeader>();
const ENTRY: usize = size_of::<ArchiveEntry>();

// A size change shows up here first. Change the version in the same edit.
const _: () = {
    assert!(ARCHIVE_MAGIC == u64::from_le_bytes(*b"SPNGPK\0\0"));
    assert!(ARCHIVE_DATA_MAGIC == u64::from_le_bytes(*b"SPNGPD\0\0"));
    assert!(size_of::<ArchiveHeader>() == 32);
    assert!(size_of::<ArchiveEntry>() == 40);
    assert!(size_of::<ArchiveDataHeader>() == 24);
};

/// The contents of `assets.spnidx`.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Index {
    pub build_id: u64,
    /// Size of each complete data file, in bytes.
    pub data_file_sizes: Vec<u64>,
    /// In the order of the name bytes.
    pub entries: Vec<(String, ArchiveEntry)>,
}

/// The one spelling of an asset name: `/` separators and no leading `/`.
pub fn normalize_name(name: &str) -> Result<String, String> {
    let name = name.replace('\\', "/");
    let name = name.trim_start_matches('/');
    if name.contains(':') || name.split('/').any(|part| matches!(part, "" | "." | "..")) {
        return Err(format!("invalid asset name: {name:?}"));
    }
    Ok(name.to_string())
}

/// The file name of data file `index`.
pub fn data_file_name(index: u32) -> String {
    format!("assets_{index:03}.spndat")
}

/// The bytes of the index. It sets `name_offset` and `name_size`. The caller
/// gives the entries in name order. Panics when a count or the names pass what
/// 32 bits can hold.
pub fn write_index(index: &Index) -> Vec<u8> {
    let count = |n: usize| u32::try_from(n).expect("the asset index passes 32 bits");
    let names_size = index.entries.iter().map(|(name, _)| name.len()).sum();
    let header = ArchiveHeader {
        magic: ARCHIVE_MAGIC.to_le_bytes(),
        version: ARCHIVE_VERSION,
        entry_count: count(index.entries.len()),
        data_file_count: count(index.data_file_sizes.len()),
        names_size: count(names_size),
        build_id: index.build_id,
    };

    let mut out = bytes_of(&header).to_vec();
    for size in &index.data_file_sizes {
        out.extend_from_slice(&size.to_le_bytes());
    }
    let mut name_offset = 0;
    for (name, entry) in &index.entries {
        let entry = ArchiveEntry {
            name_offset: count(name_offset),
            name_size: count(name.len()),
            ..*entry
        };
        out.extend_from_slice(bytes_of(&entry));
        name_offset += name.len();
    }
    for (name, _) in &index.entries {
        out.extend_from_slice(name.as_bytes());
    }
    out
}

/// The index in `bytes`. Every count, offset and name is checked.
pub fn read_index(bytes: &[u8]) -> Result<Index, String> {
    const TRUNCATED: &str = "the asset index is truncated";
    let header: ArchiveHeader = pod_read_unaligned(bytes.get(..HEADER).ok_or(TRUNCATED)?);
    if header.magic != ARCHIVE_MAGIC.to_le_bytes() {
        return Err("not an asset index".into());
    }
    if header.version != ARCHIVE_VERSION {
        return Err(format!(
            "asset index version {}, expected {ARCHIVE_VERSION}",
            header.version
        ));
    }

    // 32 bit counts times these sizes cannot overflow 64 bits.
    let sizes_end = HEADER as u64 + 8 * u64::from(header.data_file_count);
    let entries_end = sizes_end + ENTRY as u64 * u64::from(header.entry_count);
    let names_end = entries_end + u64::from(header.names_size);
    if names_end > bytes.len() as u64 {
        return Err(TRUNCATED.into());
    }
    // Each value fits in usize, because none is past the end of `bytes`.
    let (sizes_end, entries_end) = (sizes_end as usize, entries_end as usize);
    let names = &bytes[entries_end..names_end as usize];

    let data_file_sizes: Vec<u64> = bytes[HEADER..sizes_end]
        .as_chunks::<8>()
        .0
        .iter()
        .map(|b| u64::from_le_bytes(*b))
        .collect();

    let mut entries: Vec<(String, ArchiveEntry)> = Vec::new();
    for raw in bytes[sizes_end..entries_end].as_chunks::<ENTRY>().0 {
        let entry: ArchiveEntry = pod_read_unaligned(raw);
        let name = names
            .get(entry.name_offset as usize..)
            .and_then(|rest| rest.get(..entry.name_size as usize))
            .ok_or("a name runs past the end of the names")?;
        let name = std::str::from_utf8(name).map_err(|_| "a name is not UTF-8")?;
        if normalize_name(name).as_deref() != Ok(name) {
            return Err(format!("the name {name:?} is not normalized"));
        }
        if entries
            .last()
            .is_some_and(|(last, _)| last.as_str() >= name)
        {
            return Err(format!("the name {name:?} is out of order"));
        }
        let file_size = *data_file_sizes
            .get(entry.data_file as usize)
            .ok_or_else(|| format!("{name}: no data file {}", entry.data_file))?;
        if entry.compression != ARCHIVE_COMPRESSION_NONE {
            return Err(format!("{name}: unknown compression {}", entry.compression));
        }
        if entry.size != entry.raw_size {
            return Err(format!("{name}: the two sizes differ"));
        }
        if entry
            .offset
            .checked_add(entry.size)
            .is_none_or(|end| end > file_size)
        {
            return Err(format!("{name}: runs past the end of its data file"));
        }
        entries.push((name.to_string(), entry));
    }

    Ok(Index {
        build_id: header.build_id,
        data_file_sizes,
        entries,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn names_have_one_spelling() {
        for name in ["models/a.spnga", "/models/a.spnga", "models\\a.spnga"] {
            assert_eq!(normalize_name(name).unwrap(), "models/a.spnga");
        }
        assert_eq!(normalize_name("A/b").unwrap(), "A/b"); // case is kept
        for bad in ["", "/", "../a", "a/../b", "a//b", "./a", "c:/a"] {
            assert!(normalize_name(bad).is_err(), "{bad}");
        }
    }

    #[test]
    fn data_files_are_numbered_from_zero() {
        assert_eq!(data_file_name(0), "assets_000.spndat");
        assert_eq!(data_file_name(1234), "assets_1234.spndat");
    }

    /// Two entries in two data files, as `read_index` returns them.
    fn sample() -> Index {
        let entry = |name_offset, data_file, size| ArchiveEntry {
            name_offset,
            name_size: 3,
            data_file,
            compression: ARCHIVE_COMPRESSION_NONE,
            offset: ARCHIVE_ALIGNMENT,
            size,
            raw_size: size,
        };
        Index {
            build_id: 0x0123_4567_89AB_CDEF,
            data_file_sizes: vec![4106, 4116],
            entries: vec![
                ("a/b".to_string(), entry(0, 0, 10)),
                ("c/d".to_string(), entry(3, 1, 20)),
            ],
        }
    }

    // Where the parts of the sample start.
    const FIRST_ENTRY: usize = HEADER + 8 * 2;
    const NAMES: usize = FIRST_ENTRY + ENTRY * 2;

    #[test]
    fn an_index_reads_back() {
        let index = sample();
        assert_eq!(read_index(&write_index(&index)).unwrap(), index);
        let empty = Index::default();
        assert_eq!(read_index(&write_index(&empty)).unwrap(), empty);
    }

    #[test]
    fn the_layout_is_the_documented_one() {
        let bytes = write_index(&sample());
        assert_eq!(&bytes[..8], b"SPNGPK\0\0");
        assert_eq!(bytes[HEADER..HEADER + 8], 4106_u64.to_le_bytes());
        assert_eq!(&bytes[NAMES..], b"a/bc/d");
    }

    /// The error for the sample with `change` applied to its bytes.
    fn error_after(change: impl FnOnce(&mut Vec<u8>)) -> String {
        let mut bytes = write_index(&sample());
        change(&mut bytes);
        read_index(&bytes).unwrap_err()
    }

    /// The error for the sample with `change` applied before it is written.
    fn error_for(change: impl FnOnce(&mut Index)) -> String {
        let mut index = sample();
        change(&mut index);
        read_index(&write_index(&index)).unwrap_err()
    }

    #[test]
    fn a_bad_header_is_rejected() {
        assert!(error_after(|b| b[0] = b'X').contains("not an asset index"));
        assert!(error_after(|b| b[8] = 255).contains("version 255"));
        // A data file is not an index.
        let data_magic = ARCHIVE_DATA_MAGIC.to_le_bytes();
        assert!(
            error_after(|b| b[..8].copy_from_slice(&data_magic)).contains("not an asset index")
        );
    }

    #[test]
    fn a_table_past_the_end_is_rejected() {
        assert!(error_after(|b| b.truncate(HEADER - 1)).contains("truncated"));
        assert!(error_after(|b| b.truncate(b.len() - 1)).contains("truncated"));
        // The entry count, the data file count and the size of the names.
        for at in [12, 16, 20] {
            assert!(error_after(|b| b[at..at + 4].fill(0xFF)).contains("truncated"));
        }
    }

    #[test]
    fn a_name_past_the_names_is_rejected() {
        assert!(error_for_raw_name_offset(u32::MAX).contains("past the end of the names"));
        assert!(error_for_raw_name_offset(4).contains("past the end of the names"));
    }

    // `write_index` sets the name fields, so the test changes the bytes.
    fn error_for_raw_name_offset(offset: u32) -> String {
        error_after(|b| b[FIRST_ENTRY..FIRST_ENTRY + 4].copy_from_slice(&offset.to_le_bytes()))
    }

    #[test]
    fn a_bad_entry_is_rejected() {
        assert!(error_for(|i| i.entries[1].1.data_file = 2).contains("no data file 2"));
        assert!(error_for(|i| i.entries[0].1.compression = 1).contains("unknown compression"));
        assert!(error_for(|i| i.entries[0].1.raw_size = 11).contains("sizes differ"));
        assert!(error_for(|i| i.data_file_sizes[0] = 4105).contains("past the end of its data"));
        assert!(
            error_for(|i| i.entries[0].1.offset = u64::MAX).contains("past the end of its data")
        );
    }

    #[test]
    fn a_bad_name_is_rejected() {
        assert!(error_after(|b| b[NAMES] = 0xFF).contains("not UTF-8"));
        for bad in ["/ab", "a\\b", "../"] {
            assert!(error_for(|i| i.entries[0].0 = bad.to_string()).contains("not normalized"));
        }
        // Equal names and names in the wrong order.
        assert!(error_for(|i| i.entries[1].0 = "a/b".to_string()).contains("out of order"));
        assert!(error_for(|i| i.entries.swap(0, 1)).contains("out of order"));
    }
}
