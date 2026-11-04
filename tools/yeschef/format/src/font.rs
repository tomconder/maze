//! Baked font tables. A font bakes to one KTX2 file: the glyph atlas as its
//! image, and these tables under the [`FONT_KEY`] key/value entry. Written by
//! yeschef, read by `BitmapFont` through the generated header.
//!
//! Layout, little endian: a [`FontHeader`]; `slot_map_count` then
//! `tabular_map_count` [`FontSlotEntry`]; then for each size a
//! [`FontSizeHeader`], one `i32` advance per slot, [`SUBPIXEL_PHASES`]
//! [`FontGlyph`] per slot (`slot * SUBPIXEL_PHASES + phase`) and
//! `kerning_count` [`FontKerningEntry`]. Advances and kerning are 26.6 fixed
//! point, as HarfBuzz reports them.
//!
//! Shaping is table lookup: a glyph per codepoint, an advance per glyph and a
//! kerning adjustment per glyph pair. Glyphs are numbered by slot, not by font
//! glyph index. Slot 0 is the missing glyph: an unknown codepoint advances by
//! its width and draws nothing.

use std::collections::BTreeMap;

/// The key/value entry of the KTX2 file that holds the tables.
pub const FONT_KEY: &str = "spongeFont";

/// Font tables version. Increase when the layout or [`SUBPIXEL_PHASES`]
/// changes.
pub const FONT_VERSION: u32 = 1;

/// Horizontal subpixel positions baked per glyph, in 1/4 pixel steps.
pub const SUBPIXEL_PHASES: u32 = 4;

/// Start of the font tables.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct FontHeader {
    pub version: u32,
    pub slot_count: u32,
    pub slot_map_count: u32,
    pub tabular_map_count: u32,
    pub size_count: u32,
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
    pub slot_count: u32,
    /// Codepoint to slot.
    pub plain: BTreeMap<u32, u32>,
    /// Codepoint to slot, with fixed-width digits.
    pub tabular: BTreeMap<u32, u32>,
    pub sizes: Vec<SizeTable>,
}

impl Tables {
    /// The bytes of the tables.
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut out = Vec::new();
        let mut put = |bytes: &[u8]| out.extend_from_slice(bytes);
        put(bytemuck::bytes_of(&FontHeader {
            version: FONT_VERSION,
            slot_count: self.slot_count,
            slot_map_count: self.plain.len() as u32,
            tabular_map_count: self.tabular.len() as u32,
            size_count: self.sizes.len() as u32,
        }));
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
        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn size_of_the_bytes_follows_the_layout() {
        let tables = Tables {
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
        let bytes = tables.to_bytes();
        let expected = size_of::<FontHeader>()
            + 2 * size_of::<FontSlotEntry>()
            + size_of::<FontSizeHeader>()
            + 2 * 4
            + 8 * size_of::<FontGlyph>()
            + size_of::<FontKerningEntry>();
        assert_eq!(bytes.len(), expected);

        let header: FontHeader = bytemuck::pod_read_unaligned(&bytes[..size_of::<FontHeader>()]);
        assert_eq!(
            (header.version, header.slot_count, header.size_count),
            (1, 2, 1)
        );
        let at = size_of::<FontHeader>() + 2 * size_of::<FontSlotEntry>();
        let size: FontSizeHeader =
            bytemuck::pod_read_unaligned(&bytes[at..][..size_of::<FontSizeHeader>()]);
        assert_eq!((size.pixels, size.kerning_count), (12, 1));
        assert_eq!(size_of::<FontGlyph>(), 32);
    }
}
