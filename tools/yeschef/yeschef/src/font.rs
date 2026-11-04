//! Bakes a font into one KTX2 file: an RGB atlas of LCD coverage and, beside
//! it, the shaping tables the engine reads (`sponge/format/scene/src/font.hpp`
//! has the layout and the reasoning).
//!
//! Shaping at run time is table lookup: a glyph per codepoint, an advance per
//! glyph and a kerning adjustment per glyph pair. The bake shapes with
//! HarfRust to fill the tables, then shapes random strings both ways and
//! fails if the tables disagree, so a font that needs more than pair kerning
//! cannot bake by mistake.

use anyhow::{Context, Result, anyhow, bail, ensure};
use harfrust::{Buffer, Direction, Feature, ShapeOptions, ShaperFont, Tag};
use rayon::prelude::*;
use skrifa::{
    FontRef, MetadataProvider,
    instance::{LocationRef, Size},
    outline::{
        DrawSettings, Engine, HintingInstance, HintingOptions, OutlinePen, SmoothMode, Target,
    },
    raw::TableProvider,
};
use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;
use yeschef_format::font::{FONT_KEY, FontGlyph as Glyph, SUBPIXEL_PHASES, SizeTable, Tables};
use yeschef_format::ktx2::{self, Format};
use zeno::{Command, Mask, Point};

use crate::darken::{self, div_fix, mul_fix};
use crate::pack;

const PHASES: usize = SUBPIXEL_PHASES as usize;
const CHANNELS: usize = 3;
const MAX_SIDE: u32 = 2048;

/// FreeType's default five tap filter across the subpixels, summing to 256.
const LCD_FILTER: [u32; 5] = [0x08, 0x4D, 0x56, 0x4D, 0x08];
/// The filter spreads a glyph by two subpixels each side, rounded out to
/// whole pixels from the 26.6 bounding box.
const LCD_PADDING: i32 = 43;

/// Printable ASCII plus U+00D7 MULTIPLICATION SIGN.
fn charset() -> Vec<u32> {
    (32..=126).chain([0xD7]).collect()
}

/// Characters that appear in tabular figure strings (percent, resolution,
/// aspect ratio). Only these have their `tnum` glyphs drawn.
const TABULAR_CANDIDATES: [u32; 15] = [
    '0' as u32, '1' as u32, '2' as u32, '3' as u32, '4' as u32, '5' as u32, '6' as u32, '7' as u32,
    '8' as u32, '9' as u32, ':' as u32, '%' as u32, '~' as u32, ' ' as u32, 0xD7,
];

fn text_of(codepoints: &[u32]) -> String {
    codepoints
        .iter()
        .filter_map(|&c| char::from_u32(c))
        .collect()
}

// ---------------------------------------------------------------- shaping

/// A glyph from the shaper: advance and offsets are 26.6 pixels.
struct Shaped {
    glyph: u32,
    advance: i32,
    x_offset: i32,
    y_offset: i32,
}

/// The font at one size. HarfRust shapes in font units and scales the result,
/// so a scale of `pixels * 64` gives 26.6 pixels.
fn shaper(font: &harfrust::Font, pixels: u32) -> ShaperFont<'_, '_> {
    ShaperFont::new(font).with_scale(i32::try_from(pixels * 64).expect("size fits"))
}

/// Ligatures and contextual alternates are off: shaping must never ask for a
/// glyph the atlas does not hold (Inter's `calt` swaps multiply for
/// multiply.case). `tnum` only on request.
fn shape(shaper: &ShaperFont, text: &str, tabular: bool) -> Result<Vec<Shaped>> {
    let features = [
        Feature::new(Tag::new(b"liga"), 0, ..),
        Feature::new(Tag::new(b"clig"), 0, ..),
        Feature::new(Tag::new(b"calt"), 0, ..),
        Feature::new(Tag::new(b"tnum"), u32::from(tabular), ..),
    ];
    let mut buffer = Buffer::new();
    buffer.push_str(text);
    buffer.set_direction(Direction::LeftToRight);
    buffer.guess_segment_properties();
    harfrust::shape(shaper, &mut buffer, ShapeOptions::new().features(&features))
        .map_err(|e| anyhow!("shaping failed: {e}"))?;
    Ok(buffer
        .glyph_infos()
        .iter()
        .zip(buffer.glyph_positions())
        .map(|(info, position)| Shaped {
            glyph: info.glyph_id,
            advance: position.x_advance,
            x_offset: position.x_offset,
            y_offset: position.y_offset,
        })
        .collect())
}

/// Glyph index to slot, in order of first use. Slot 0 is the missing glyph.
struct Slots {
    of_glyph: BTreeMap<u32, u32>,
    glyphs: Vec<u32>,
    plain: BTreeMap<u32, u32>,
    tabular: BTreeMap<u32, u32>,
}

impl Slots {
    fn new(shaper: &ShaperFont) -> Result<Self> {
        let mut slots = Slots {
            of_glyph: BTreeMap::from([(0, 0)]),
            glyphs: vec![0],
            plain: BTreeMap::new(),
            tabular: BTreeMap::new(),
        };
        for tabular in [false, true] {
            for codepoint in charset() {
                let shaped = shape(shaper, &text_of(&[codepoint]), tabular)?;
                ensure!(
                    shaped.len() == 1,
                    "U+{codepoint:04X} does not shape to one glyph"
                );
                let next = u32::try_from(slots.glyphs.len())?;
                let slot = *slots.of_glyph.entry(shaped[0].glyph).or_insert(next);
                if slot == next {
                    slots.glyphs.push(shaped[0].glyph);
                }
                let map = if tabular {
                    &mut slots.tabular
                } else {
                    &mut slots.plain
                };
                map.insert(codepoint, slot);
            }
        }
        Ok(slots)
    }

    fn map(&self, tabular: bool) -> &BTreeMap<u32, u32> {
        if tabular { &self.tabular } else { &self.plain }
    }
}

// ----------------------------------------------------------------- tables

/// What the engine does at run time: a slot per codepoint, an advance per
/// slot, kerning from the pair before. Advances are in pixels.
fn table_shape(tables: &Tables, size: &SizeTable, text: &[u32], tabular: bool) -> Vec<(u32, f32)> {
    let slots = if tabular {
        &tables.tabular
    } else {
        &tables.plain
    };
    let mut glyphs: Vec<(u32, f32)> = Vec::new();
    for codepoint in text {
        let slot = slots.get(codepoint).copied().unwrap_or(0);
        if let Some(last) = glyphs.last_mut()
            && let Some(&kerning) = size.kerning.get(&((last.0 << 16) | slot))
        {
            last.1 += kerning as f32 / 64.0;
        }
        glyphs.push((slot, size.advances[slot as usize] as f32 / 64.0));
    }
    glyphs
}

/// The advance of every slot and the kerning of every pair at one size, from
/// single codepoint and pair shaping.
fn size_tables(
    font: &harfrust::Font,
    slots: &Slots,
    pixels: u32,
) -> Result<(Vec<i32>, BTreeMap<u32, i32>)> {
    let shaper = shaper(font, pixels);
    let codepoints = charset();
    let mut advances = BTreeMap::from([(0, shaper.glyph_h_advance(harfrust::GlyphId::new(0)))]);

    for tabular in [false, true] {
        for &codepoint in &codepoints {
            let shaped = shape(&shaper, &text_of(&[codepoint]), tabular)?;
            if shaped.len() != 1 || shaped[0].x_offset != 0 || shaped[0].y_offset != 0 {
                bail!("U+{codepoint:04X} does not shape to one unshifted glyph");
            }
            let slot = slots.map(tabular)[&codepoint];
            let known = *advances.entry(slot).or_insert(shaped[0].advance);
            ensure!(
                known == shaped[0].advance,
                "glyph {} advances differently with tabular figures",
                shaped[0].glyph
            );
        }
    }

    let mut kerning = BTreeMap::new();
    for tabular in [false, true] {
        let map = slots.map(tabular);
        for &first in &codepoints {
            for &second in &codepoints {
                let shaped = shape(&shaper, &text_of(&[first, second]), tabular)?;
                let (first_slot, second_slot) = (map[&first], map[&second]);
                if shaped.len() != 2
                    || shaped.iter().any(|g| g.x_offset != 0 || g.y_offset != 0)
                    || shaped[1].advance != advances[&second_slot]
                {
                    bail!("U+{first:04X} U+{second:04X} needs more than pair kerning");
                }
                let adjustment = shaped[0].advance - advances[&first_slot];
                let known = *kerning
                    .entry((first_slot << 16) | second_slot)
                    .or_insert(adjustment);
                ensure!(
                    known == adjustment,
                    "a glyph pair kerns differently with tabular figures"
                );
            }
        }
    }
    kerning.retain(|_, adjustment| *adjustment != 0);

    let advances = (0..slots.glyphs.len() as u32)
        .map(|slot| advances[&slot])
        .collect();
    Ok((advances, kerning))
}

struct Random(u64);

impl Random {
    /// A fixed sequence: the bake must be reproducible.
    fn below(&mut self, bound: usize) -> usize {
        self.0 = self
            .0
            .wrapping_mul(6_364_136_223_846_793_005)
            .wrapping_add(1_442_695_040_888_963_407);
        ((self.0 >> 33) as usize) % bound
    }
}

/// Shapes random strings both ways at one size.
fn check_against_shaper(
    font: &harfrust::Font,
    slots: &Slots,
    tables: &Tables,
    size: &SizeTable,
) -> Result<()> {
    const STRINGS: usize = 256;
    const MAX_LENGTH: usize = 48;

    let shaper = shaper(font, size.pixels);
    let codepoints = charset();
    let mut random = Random(u64::from(size.pixels));
    for _ in 0..STRINGS {
        let text: Vec<u32> = (0..1 + random.below(MAX_LENGTH))
            .map(|_| codepoints[random.below(codepoints.len())])
            .collect();
        for tabular in [false, true] {
            let want = shape(&shaper, &text_of(&text), tabular)?;
            let got = table_shape(tables, size, &text, tabular);
            let same = want.len() == got.len()
                && want.iter().zip(&got).all(|(want, got)| {
                    slots.of_glyph.get(&want.glyph) == Some(&got.0)
                        && want.advance as f32 / 64.0 == got.1
                        && want.x_offset == 0
                        && want.y_offset == 0
                });
            ensure!(
                same,
                "baked shaping differs from the shaper at {} px for \"{}\"",
                size.pixels,
                text_of(&text)
            );
        }
    }
    Ok(())
}

/// The line height and ascender FreeType reports for a pixel size: the ascent
/// rounded up, the line height rounded to a whole pixel.
fn line_metrics(font: &FontRef, pixels: u32) -> Result<(f32, f32)> {
    let hhea = font.hhea().context("font has no hhea table")?;
    let upem = i64::from(
        font.head()
            .context("font has no head table")?
            .units_per_em(),
    );
    let ascent = i64::from(hhea.ascender().to_i16());
    let descent = i64::from(hhea.descender().to_i16());
    let gap = i64::from(hhea.line_gap().to_i16());
    let scale = div_fix(i64::from(pixels) << 6, upem);

    let ascender = (mul_fix(ascent, scale) + 63) & !63;
    let height = (mul_fix(ascent - descent + gap, scale) + 32) & !63;
    Ok(((height >> 6) as f32, (ascender >> 6) as f32))
}

// ---------------------------------------------------------------- drawing

struct Pen(Vec<Command>);

impl OutlinePen for Pen {
    fn move_to(&mut self, x: f32, y: f32) {
        self.0.push(Command::MoveTo(Point::new(x, y)));
    }
    fn line_to(&mut self, x: f32, y: f32) {
        self.0.push(Command::LineTo(Point::new(x, y)));
    }
    fn quad_to(&mut self, cx: f32, cy: f32, x: f32, y: f32) {
        self.0
            .push(Command::QuadTo(Point::new(cx, cy), Point::new(x, y)));
    }
    fn curve_to(&mut self, ax: f32, ay: f32, bx: f32, by: f32, x: f32, y: f32) {
        self.0.push(Command::CurveTo(
            Point::new(ax, ay),
            Point::new(bx, by),
            Point::new(x, y),
        ));
    }
    fn close(&mut self) {
        self.0.push(Command::Close);
    }
}

/// A drawn glyph: where it sits and its LCD coverage, three bytes a pixel.
struct Drawn {
    glyph: Glyph,
    rgb: Vec<u8>,
}

fn control_points(command: &Command) -> Vec<Point> {
    match *command {
        Command::MoveTo(p) | Command::LineTo(p) => vec![p],
        Command::QuadTo(a, b) => vec![a, b],
        Command::CurveTo(a, b, c) => vec![a, b, c],
        Command::Close => vec![],
    }
}

/// Draws an outline (pixels, y up) moved right by `phase / 4` pixel. The
/// bitmap is the outline's 26.6 bounding box widened for the filter and
/// rounded out to whole pixels, so its size and bearings are FreeType's.
fn draw(outline: &[Command], phase: usize) -> Drawn {
    let shift = phase as f32 / 4.0;
    let (mut x0, mut y0, mut x1, mut y1) = (i32::MAX, i32::MAX, i32::MIN, i32::MIN);
    for point in outline.iter().flat_map(control_points) {
        let x = ((point.x + shift) * 64.0).round() as i32;
        let y = (point.y * 64.0).round() as i32;
        (x0, x1, y0, y1) = (x0.min(x), x1.max(x), y0.min(y), y1.max(y));
    }
    if x0 > x1 {
        (x0, x1, y0, y1) = (0, 0, 0, 0);
    }
    let floor = |v: i32| v.div_euclid(64);
    let ceil = |v: i32| (v + 63).div_euclid(64);
    let (left, right) = (floor(x0 - LCD_PADDING), ceil(x1 + LCD_PADDING));
    let (bottom, top) = (floor(y0), ceil(y1));
    let (width, height) = ((right - left) as usize, (top - bottom) as usize);

    let glyph = Glyph {
        bearing_x: left,
        bearing_y: top,
        width: width as i32,
        height: height as i32,
        ..Glyph::default()
    };
    if height == 0 {
        return Drawn {
            glyph,
            rgb: Vec::new(),
        };
    }

    // Three times as wide, so each subpixel gets its own coverage.
    let to_bitmap = |p: Point| Point::new((p.x + shift - left as f32) * 3.0, top as f32 - p.y);
    let moved: Vec<Command> = outline
        .iter()
        .map(|command| match *command {
            Command::MoveTo(p) => Command::MoveTo(to_bitmap(p)),
            Command::LineTo(p) => Command::LineTo(to_bitmap(p)),
            Command::QuadTo(a, b) => Command::QuadTo(to_bitmap(a), to_bitmap(b)),
            Command::CurveTo(a, b, c) => Command::CurveTo(to_bitmap(a), to_bitmap(b), to_bitmap(c)),
            Command::Close => Command::Close,
        })
        .collect();
    let row_len = width * CHANNELS;
    let (coverage, _) = Mask::new(&moved[..])
        .size(row_len as u32, height as u32)
        .render();

    let mut rgb = vec![0u8; row_len * height];
    for (row, out) in coverage
        .chunks_exact(row_len)
        .zip(rgb.chunks_exact_mut(row_len))
    {
        for (i, out) in out.iter_mut().enumerate() {
            let sum: u32 = LCD_FILTER
                .iter()
                .enumerate()
                .filter_map(|(tap, weight)| {
                    let at = (i + tap).checked_sub(2)?;
                    Some(weight * u32::from(*row.get(at)?))
                })
                .sum();
            *out = (sum >> 8).min(255) as u8;
        }
    }
    Drawn { glyph, rgb }
}

/// Draws every wanted glyph at one size, at each phase. The outline is hinted
/// for light anti-aliasing (vertical only), then darkened.
fn draw_size(
    font: &FontRef,
    glyphs: &BTreeSet<u32>,
    pixels: u32,
    standard_width: i64,
    units_per_em: u32,
) -> Result<BTreeMap<(u32, usize), Drawn>> {
    let outlines = font.outline_glyphs();
    let hinting = HintingInstance::new(
        &outlines,
        Size::new(pixels as f32),
        LocationRef::default(),
        HintingOptions {
            engine: Engine::Auto(None),
            target: Target::Smooth {
                mode: SmoothMode::Light,
                symmetric_rendering: true,
                preserve_linear_metrics: false,
            },
        },
    )
    .map_err(|e| anyhow!("unable to hint at {pixels} px: {e}"))?;
    let darkening = darken::amount_x(pixels, units_per_em, standard_width);

    let mut drawn = BTreeMap::new();
    for &glyph in glyphs {
        let mut pen = Pen(Vec::new());
        outlines
            .get(skrifa::GlyphId::new(glyph))
            .with_context(|| format!("glyph {glyph} has no outline"))?
            .draw(DrawSettings::hinted(&hinting, false), &mut pen)
            .map_err(|e| anyhow!("unable to draw glyph {glyph} at {pixels} px: {e}"))?;
        let outline = darken::embolden_x(&pen.0, darkening);
        for phase in 0..PHASES {
            drawn.insert((glyph, phase), draw(&outline, phase));
        }
    }
    Ok(drawn)
}

// ------------------------------------------------------------------ atlas

/// Packs every drawn bitmap into one atlas, each with a zero texel border on
/// its right and bottom. Returns the atlas and the glyph table by
/// `(size index, glyph, phase)`.
#[allow(clippy::type_complexity)]
fn build_atlas(
    drawn: Vec<BTreeMap<(u32, usize), Drawn>>,
) -> Result<((u32, u32), Vec<u8>, BTreeMap<(usize, u32, usize), Glyph>)> {
    let mut keys = Vec::new();
    let mut bitmaps = Vec::new();
    let mut placed = BTreeMap::new();
    for (size, glyphs) in drawn.into_iter().enumerate() {
        for ((glyph, phase), drawn) in glyphs {
            if drawn.rgb.is_empty() {
                // Space and the like need no room in the atlas.
                placed.insert((size, glyph, phase), drawn.glyph);
            } else {
                keys.push((size, glyph, phase));
                bitmaps.push(drawn);
            }
        }
    }

    let rects: Vec<(u32, u32)> = bitmaps
        .iter()
        .map(|d| (d.glyph.width as u32 + 1, d.glyph.height as u32 + 1))
        .collect();
    let Some(((width, height), places)) = pack::pack(&rects, MAX_SIDE) else {
        bail!(
            "{} glyphs do not fit in a {MAX_SIDE}x{MAX_SIDE} atlas",
            rects.len()
        );
    };

    let mut pixels = vec![0u8; width as usize * height as usize * CHANNELS];
    for ((key, drawn), place) in keys.into_iter().zip(bitmaps).zip(places) {
        let row_len = drawn.glyph.width as usize * CHANNELS;
        for (row, line) in drawn.rgb.chunks_exact(row_len).enumerate() {
            let to = ((place.y as usize + row) * width as usize + place.x as usize) * CHANNELS;
            pixels[to..to + row_len].copy_from_slice(line);
        }
        let (w, h) = (width as f32, height as f32);
        placed.insert(
            key,
            Glyph {
                uv_left: place.x as f32 / w,
                uv_top: place.y as f32 / h,
                uv_width: drawn.glyph.width as f32 / w,
                uv_height: drawn.glyph.height as f32 / h,
                ..drawn.glyph
            },
        );
    }
    Ok(((width, height), pixels, placed))
}

// ---------------------------------------------------------------- convert

/// The width of the vertical stems of "o" in font units, which sets how much
/// the font darkens. FreeType finds this by analysing the same glyph's edges;
/// the stroke thickness at mid height is the same measure to a few units.
fn standard_width(font: &FontRef) -> i64 {
    let Some(glyph) = font.charmap().map('o') else {
        return 0;
    };
    let mut pen = Pen(Vec::new());
    let drawn = font.outline_glyphs().get(glyph).map(|outline| {
        outline.draw(
            DrawSettings::unhinted(Size::unscaled(), LocationRef::default()),
            &mut pen,
        )
    });
    match drawn {
        Some(Ok(_)) => darken::standard_width(&pen.0),
        _ => 0,
    }
}

/// Bakes the font at each size. Returns the KTX2 file and the atlas size.
pub fn bake(data: &[u8], sizes: &[u32]) -> Result<(Vec<u8>, (u32, u32))> {
    ensure!(!sizes.is_empty(), "no font sizes");
    ensure!(sizes.iter().all(|&s| s > 0), "a font size is 0");

    let font = FontRef::new(data).context("unable to read the font")?;
    let shaper_font = harfrust::Font::new(data.to_vec(), 0).context("unable to read the font")?;
    let upem = u32::from(
        font.head()
            .context("font has no head table")?
            .units_per_em(),
    );
    let slots = Slots::new(&shaper(&shaper_font, sizes[0]))?;

    // Every glyph the tables can point at: the codepoint glyphs and the
    // tabular figure glyphs of the candidates. Glyph 0 draws nothing.
    let mut wanted: BTreeSet<u32> = slots.glyphs.iter().copied().collect();
    let candidates = shape(
        &shaper(&shaper_font, sizes[0]),
        &text_of(&TABULAR_CANDIDATES),
        true,
    )?;
    wanted.extend(candidates.iter().map(|g| g.glyph));
    wanted.extend(
        charset()
            .iter()
            .filter_map(|&c| char::from_u32(c))
            .filter_map(|c| font.charmap().map(c).map(|g| g.to_u32())),
    );
    wanted.remove(&0);

    let std_width = standard_width(&font);
    let drawn = sizes
        .par_iter()
        .map(|&pixels| draw_size(&font, &wanted, pixels, std_width, upem))
        .collect::<Result<Vec<_>>>()?;
    let ((width, height), pixels, placed) = build_atlas(drawn)?;

    let size_tables = sizes
        .par_iter()
        .enumerate()
        .map(|(index, &pixels)| -> Result<SizeTable> {
            let (advances, kerning) = size_tables(&shaper_font, &slots, pixels)
                .with_context(|| format!("at {pixels} px"))?;
            let (line_height, ascender) = line_metrics(&font, pixels)?;
            let glyphs = slots
                .glyphs
                .iter()
                .flat_map(|&glyph| (0..PHASES).map(move |phase| (glyph, phase)))
                .map(|(glyph, phase)| {
                    placed
                        .get(&(index, glyph, phase))
                        .copied()
                        .unwrap_or_default()
                })
                .collect();
            Ok(SizeTable {
                pixels,
                line_height,
                ascender,
                advances,
                glyphs,
                kerning,
            })
        })
        .collect::<Result<Vec<_>>>()?;

    let tables = Tables {
        slot_count: u32::try_from(slots.glyphs.len())?,
        plain: slots.plain.clone(),
        tabular: slots.tabular.clone(),
        sizes: size_tables,
    };
    tables
        .sizes
        .par_iter()
        .try_for_each(|size| check_against_shaper(&shaper_font, &slots, &tables, size))?;

    let key_values = [(FONT_KEY.to_string(), tables.to_bytes())];
    let file = ktx2::write(Format::R8G8B8Unorm, width, height, &[pixels], &key_values);
    Ok((file, (width, height)))
}

pub fn convert(from: &Path, to: &Path, sizes: &[u32]) -> Result<()> {
    let data = std::fs::read(from).with_context(|| format!("unable to read {}", from.display()))?;
    let (file, (width, height)) =
        bake(&data, sizes).with_context(|| format!("unable to bake {}", from.display()))?;
    std::fs::write(to, &file).with_context(|| format!("unable to write {}", to.display()))?;
    println!(
        "{} sizes -> {} ({width}x{height}, {} bytes)",
        sizes.len(),
        to.to_string_lossy().replace('\\', "/"),
        file.len()
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn inter() -> Vec<u8> {
        let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/fonts/inter.ttf");
        std::fs::read(path).expect("the game's font")
    }

    fn u32_at(bytes: &[u8], at: usize) -> u32 {
        u32::from_le_bytes(bytes[at..at + 4].try_into().unwrap())
    }

    #[test]
    fn a_baked_font_has_the_tables_the_engine_reads() {
        let (file, (width, height)) = bake(&inter(), &[12]).unwrap();
        let image = ktx2::read(&file).unwrap();
        assert_eq!(image.vk_format, Format::R8G8B8Unorm as u32);
        assert_eq!((image.width, image.height), (width, height));
        assert_eq!(image.levels[0].len(), (width * height) as usize * CHANNELS);
        assert!(image.levels[0].iter().any(|&texel| texel != 0));

        let (key, tables) = &image.key_values[0];
        assert_eq!(key, FONT_KEY);
        let (version, slots, plain, tabular, sizes) = (
            u32_at(tables, 0),
            u32_at(tables, 4) as usize,
            u32_at(tables, 8),
            u32_at(tables, 12),
            u32_at(tables, 16),
        );
        assert_eq!(
            (version, plain, tabular, sizes),
            (yeschef_format::font::FONT_VERSION, 96, 96, 1)
        );
        // Printable ASCII, the multiplication sign, the missing glyph, and a
        // few tabular figure forms.
        assert!((96..=140).contains(&slots), "{slots} slots");

        // Size header: pixels, line height 15 and ascender 12 for Inter at
        // 12 px, as FreeType reports them.
        let size_at = 20 + 8 * (plain + tabular) as usize;
        assert_eq!(u32_at(tables, size_at), 12);
        assert_eq!(f32::from_bits(u32_at(tables, size_at + 4)), 15.0);
        assert_eq!(f32::from_bits(u32_at(tables, size_at + 8)), 12.0);
        // Slot 0 is the missing glyph: it draws nothing.
        let first_glyph = size_at + 16 + 4 * slots;
        assert_eq!(&tables[first_glyph..first_glyph + 32], &[0u8; 32]);
    }

    #[test]
    fn bigger_sizes_have_bigger_advances() {
        let (file, _) = bake(&inter(), &[12, 24]).unwrap();
        let image = ktx2::read(&file).unwrap();
        let tables = image.key_values[0].1;
        let slots = u32_at(tables, 4) as usize;
        let first = 20 + 8 * 192;
        let kerning_12 = u32_at(tables, first + 12) as usize;
        let second = first + 16 + 4 * slots + 32 * PHASES * slots + 8 * kerning_12;
        assert_eq!(u32_at(tables, second), 24);
        // The space slot is the same slot at both sizes, twice as wide.
        let advance = |size_at: usize| u32_at(tables, size_at + 16 + 4);
        assert_eq!(advance(second), advance(first) * 2);
    }

    #[test]
    fn a_size_of_zero_is_an_error() {
        assert!(bake(&inter(), &[0]).is_err());
        assert!(bake(&inter(), &[]).is_err());
    }

    #[test]
    fn something_that_is_not_a_font_is_an_error() {
        assert!(bake(b"not a font", &[12]).is_err());
    }

    #[test]
    fn the_engine_lookup_applies_kerning_to_the_first_glyph() {
        let size = SizeTable {
            pixels: 10,
            line_height: 0.0,
            ascender: 0.0,
            advances: vec![640, 640, 320],
            glyphs: Vec::new(),
            kerning: BTreeMap::from([((1 << 16) | 2, -64)]),
        };
        let tables = Tables {
            slot_count: 3,
            plain: BTreeMap::from([(65, 1), (66, 2)]),
            tabular: BTreeMap::new(),
            sizes: Vec::new(),
        };
        let shaped = table_shape(&tables, &size, &[65, 66, 67], false);
        // A kerns with B; C is unknown and takes slot 0.
        assert_eq!(shaped, vec![(1, 9.0), (2, 5.0), (0, 10.0)]);
    }

    #[test]
    fn a_blank_glyph_has_filter_padding_and_no_rows() {
        let drawn = draw(&[], 0);
        assert_eq!(
            (drawn.glyph.bearing_x, drawn.glyph.width, drawn.glyph.height),
            (-1, 2, 0)
        );
        assert!(drawn.rgb.is_empty());
    }
}
