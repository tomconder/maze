//! The baked font file (`.spnga`): the shaping tables, then the glyph atlas
//! as a whole KTX2 file. Written by yeschef, read by `BitmapFont` through the
//! generated header.
//!
//! Layout, little endian: a [`FontHeader`]; `slot_map_count` then
//! `tabular_map_count` [`FontSlotEntry`]; then for each size a
//! [`FontSizeHeader`], one `i32` advance per slot, [`SUBPIXEL_PHASES`]
//! [`FontGlyph`] per slot (`slot * SUBPIXEL_PHASES + phase`) and
//! `kerning_count` [`FontKerningEntry`]. Advances and kerning are 26.6 fixed
//! point, as HarfBuzz reports them. The KTX2 file follows the last size and
//! ends the file.
//!
//! Shaping is table lookup: a glyph per codepoint, an advance per glyph and a
//! kerning adjustment per glyph pair. Glyphs are numbered by slot, not by font
//! glyph index. Slot 0 is the missing glyph: an unknown codepoint advances by
//! its width and draws nothing.

use std::collections::BTreeMap;

/// Font file magic, the bytes `SPNGF\0\0\0` read as a little endian integer.
pub const FONT_MAGIC: u64 = 0x0000_0046_474E_5053;

/// Font file version. Increase when the layout or [`SUBPIXEL_PHASES`]
/// changes.
pub const FONT_VERSION: u32 = 3;

/// Horizontal subpixel positions baked per glyph, in 1/4 pixel steps.
pub const SUBPIXEL_PHASES: u32 = 4;

/// Start of a font file.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct FontHeader {
    /// The bytes of `FONT_MAGIC`.
    pub magic: [u8; 8],
    pub version: u32,
    pub slot_count: u32,
    pub slot_map_count: u32,
    pub tabular_map_count: u32,
    pub size_count: u32,
    /// The KTX2 atlas, from the start of the file.
    pub ktx2_offset: u32,
    pub ktx2_size: u32,
    /// [`hash`] of the source font file, little endian.
    pub source_hash: [u8; 8],
}

/// A codepoint and its glyph slot.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct FontSlotEntry {
    pub codepoint: u32,
    pub slot: u32,
}

/// Start of one size.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct FontSizeHeader {
    pub pixels: u32,
    pub line_height: f32,
    pub ascender: f32,
    pub kerning_count: u32,
}

/// The change in the first advance of a glyph pair.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct FontKerningEntry {
    /// `first slot << 16 | second slot`.
    pub pair: u32,
    pub adjustment: i32,
}

/// Atlas placement of one glyph at one size and phase. A width or height of 0
/// means nothing to draw.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct FontGlyph {
    pub uv_left: f32,
    pub uv_top: f32,
    pub uv_width: f32,
    pub uv_height: f32,
    pub bearing_x: i32,
    pub bearing_y: i32,
    pub width: i32,
    pub height: i32,
}

pub struct SizeTable {
    pub pixels: u32,
    pub line_height: f32,
    pub ascender: f32,
    /// 26.6 pixels, per slot.
    pub advances: Vec<i32>,
    /// `slot * SUBPIXEL_PHASES + phase`.
    pub glyphs: Vec<FontGlyph>,
    /// `(first slot << 16 | second slot)` to the change in the first advance,
    /// 26.6 pixels.
    pub kerning: BTreeMap<u32, i32>,
}

pub struct Tables {
    /// [`hash`] of the source font file.
    pub source_hash: u64,
    pub slot_count: u32,
    /// Codepoint to slot.
    pub plain: BTreeMap<u32, u32>,
    /// Codepoint to slot, with fixed-width digits.
    pub tabular: BTreeMap<u32, u32>,
    pub sizes: Vec<SizeTable>,
}

impl Tables {
    /// The bytes of the font file, with `ktx2` stored whole at the end.
    pub fn to_file(&self, ktx2: &[u8]) -> Vec<u8> {
        // The header needs the size of the tables, so it is filled in last.
        let mut out = vec![0; size_of::<FontHeader>()];
        let mut put = |bytes: &[u8]| out.extend_from_slice(bytes);
        for map in [&self.plain, &self.tabular] {
            for (&codepoint, &slot) in map {
                put(bytemuck::bytes_of(&FontSlotEntry { codepoint, slot }));
            }
        }
        for size in &self.sizes {
            put(bytemuck::bytes_of(&FontSizeHeader {
                pixels: size.pixels,
                line_height: size.line_height,
                ascender: size.ascender,
                kerning_count: size.kerning.len() as u32,
            }));
            put(bytemuck::cast_slice(&size.advances));
            put(bytemuck::cast_slice(&size.glyphs));
            for (&pair, &adjustment) in &size.kerning {
                put(bytemuck::bytes_of(&FontKerningEntry { pair, adjustment }));
            }
        }
        let header = FontHeader {
            magic: FONT_MAGIC.to_le_bytes(),
            version: FONT_VERSION,
            slot_count: self.slot_count,
            slot_map_count: self.plain.len() as u32,
            tabular_map_count: self.tabular.len() as u32,
            size_count: self.sizes.len() as u32,
            ktx2_offset: u32::try_from(out.len()).expect("fits in 32 bits"),
            ktx2_size: u32::try_from(ktx2.len()).expect("fits in 32 bits"),
            source_hash: self.source_hash.to_le_bytes(),
        };
        out[..size_of::<FontHeader>()].copy_from_slice(bytemuck::bytes_of(&header));
        out.extend_from_slice(ktx2);
        out
    }
}

/// 64 bit FNV-1a. Saved IDs depend on it, so it must not change, which rules
/// out the standard library hasher.
pub fn hash<'a>(bytes: impl IntoIterator<Item = &'a u8>) -> u64 {
    bytes
        .into_iter()
        .fold(0xcbf2_9ce4_8422_2325, |hash, &byte| {
            (hash ^ u64::from(byte)).wrapping_mul(0x0100_0000_01b3)
        })
}

/// Identifies what an atlas was drawn for: the source font, the sizes and every
/// glyph table. The atlas itself does not count, so an edit keeps the ID.
pub fn layout_id(file: &[u8]) -> Result<String, String> {
    let (tables, _) = split(file)?;
    let size_at = std::mem::offset_of!(FontHeader, ktx2_size);
    let kept = tables[..size_at].iter().chain(&tables[size_at + 4..]);
    Ok(format!("{:016x}", hash(kept)))
}

/// Splits a font file into the bytes before the atlas and the KTX2 atlas.
pub fn split(file: &[u8]) -> Result<(&[u8], &[u8]), String> {
    let header: FontHeader = file
        .get(..size_of::<FontHeader>())
        .map(bytemuck::pod_read_unaligned)
        .ok_or("the font file is truncated")?;
    if header.magic != FONT_MAGIC.to_le_bytes() {
        return Err("not a font file".into());
    }
    if header.version != FONT_VERSION {
        return Err(format!(
            "font file version {}, expected {FONT_VERSION}",
            header.version
        ));
    }
    let offset = header.ktx2_offset as usize;
    if offset < size_of::<FontHeader>()
        || offset > file.len()
        || header.ktx2_size as usize != file.len() - offset
    {
        return Err("the font atlas does not end the file".into());
    }
    Ok(file.split_at(offset))
}

/// The font file with `ktx2` in place of its atlas. The tables do not change.
pub fn replace_atlas(file: &[u8], ktx2: &[u8]) -> Result<Vec<u8>, String> {
    let (tables, _) = split(file)?;
    let mut header: FontHeader = bytemuck::pod_read_unaligned(&tables[..size_of::<FontHeader>()]);
    header.ktx2_size = u32::try_from(ktx2.len()).map_err(|_| "the atlas is over 4 GiB")?;
    let mut out = [tables, ktx2].concat();
    out[..size_of::<FontHeader>()].copy_from_slice(bytemuck::bytes_of(&header));
    Ok(out)
}

const _: () = {
    assert!(FONT_MAGIC == u64::from_le_bytes(*b"SPNGF\0\0\0"));
    assert!(size_of::<FontHeader>() == 44);
};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_atlas_can_be_replaced() {
        let tables = Tables {
            source_hash: 0,
            slot_count: 0,
            plain: BTreeMap::new(),
            tabular: BTreeMap::new(),
            sizes: Vec::new(),
        };
        let file = tables.to_file(b"old");
        let replaced = replace_atlas(&file, b"longer").unwrap();
        assert_eq!(replaced, tables.to_file(b"longer"));
        assert_eq!(split(&replaced).unwrap().1, b"longer");
        assert!(split(&file[..file.len() - 1]).is_err());
        assert!(split(b"short").is_err());

        assert_eq!(layout_id(&file).unwrap(), layout_id(&replaced).unwrap());
        let other = Tables {
            source_hash: 1,
            ..tables
        };
        assert_ne!(
            layout_id(&file).unwrap(),
            layout_id(&other.to_file(b"old")).unwrap()
        );
        // The published FNV-1a value of "a".
        assert_eq!(hash(b"a"), 0xaf63_dc4c_8601_ec8c);
    }

    #[test]
    fn size_of_the_bytes_follows_the_layout() {
        let tables = Tables {
            source_hash: 0,
            slot_count: 2,
            plain: BTreeMap::from([(65, 1)]),
            tabular: BTreeMap::from([(48, 1)]),
            sizes: vec![SizeTable {
                pixels: 12,
                line_height: 15.0,
                ascender: 12.0,
                advances: vec![640, 700],
                glyphs: vec![FontGlyph::default(); 2 * SUBPIXEL_PHASES as usize],
                kerning: BTreeMap::from([((1 << 16) | 1, -64)]),
            }],
        };
        let bytes = tables.to_file(b"atlas");
        let expected = size_of::<FontHeader>()
            + 2 * size_of::<FontSlotEntry>()
            + size_of::<FontSizeHeader>()
            + 2 * 4
            + 8 * size_of::<FontGlyph>()
            + size_of::<FontKerningEntry>();
        assert_eq!(bytes.len(), expected + 5);
        assert_eq!(&bytes[expected..], b"atlas");

        let header: FontHeader = bytemuck::pod_read_unaligned(&bytes[..size_of::<FontHeader>()]);
        assert_eq!(
            (header.version, header.slot_count, header.size_count),
            (FONT_VERSION, 2, 1)
        );
        assert_eq!(
            (header.ktx2_offset as usize, header.ktx2_size),
            (expected, 5)
        );
        let at = size_of::<FontHeader>() + 2 * size_of::<FontSlotEntry>();
        let size: FontSizeHeader =
            bytemuck::pod_read_unaligned(&bytes[at..][..size_of::<FontSizeHeader>()]);
        assert_eq!((size.pixels, size.kerning_count), (12, 1));
        assert_eq!(size_of::<FontGlyph>(), 32);
    }
}
