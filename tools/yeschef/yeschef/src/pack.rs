//! Rectangle packing for the sprite atlas and, later, the font atlas.

use rectangle_pack::{
    GroupedRectsToPlace, RectToInsert, TargetBin, contains_smallest_box, pack_rects,
    volume_heuristic,
};
use std::collections::BTreeMap;

/// Where a rectangle landed, by its index in the input.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Placement {
    pub x: u32,
    pub y: u32,
}

/// The result of one packing: the bounding size of what was placed, and a
/// placement for each input rectangle.
type Packing = ((u32, u32), Vec<Placement>);

fn area_of(packing: &Packing) -> u64 {
    u64::from(packing.0.0) * u64::from(packing.0.1)
}

/// Packs the `(width, height)` rectangles into the smallest bin, of any
/// shape, that fits them, of at most `max_side` on each side. Returns the
/// bounding size of what was placed and a placement for each input rectangle.
/// Returns `None` when they do not fit. Rectangles never rotate: sprites and
/// glyphs are sampled axis aligned.
///
/// The search tries about fifty bin widths and, for each, bisects the least
/// height that fits; then it tries widths near the best one more closely. It
/// assumes that a bin which fits the rectangles still fits them when it grows.
/// The packer is greedy, so that can fail in theory; the result is then a
/// little bigger than the best one.
pub fn pack(sizes: &[(u32, u32)], max_side: u32) -> Option<Packing> {
    if sizes.is_empty() {
        return Some(((0, 0), Vec::new()));
    }
    let area: u64 = sizes
        .iter()
        .map(|&(w, h)| u64::from(w) * u64::from(h))
        .sum();
    let widest = sizes.iter().map(|&(w, _)| w).max().unwrap();
    let tallest = sizes.iter().map(|&(_, h)| h).max().unwrap();
    if widest > max_side || tallest > max_side {
        return None;
    }

    // A narrower bin than this cannot be short enough.
    let narrowest = widest.max(area.div_ceil(u64::from(max_side)) as u32);
    if narrowest > max_side {
        return None; // the total area is more than the largest bin holds
    }
    let step = ((max_side - narrowest) / 48).max(1);

    let mut best: Option<Packing> = None;
    let try_width = |best: &mut Option<Packing>, width: u32| {
        if let Some(found) = fit_width(sizes, area, tallest, width, max_side)
            && best.as_ref().is_none_or(|b| area_of(&found) < area_of(b))
        {
            *best = Some(found);
        }
    };
    let mut width = narrowest;
    while width < max_side {
        try_width(&mut best, width);
        width += step;
    }
    try_width(&mut best, max_side);

    // Look closer around the best width so far.
    let near = best.as_ref()?.0.0;
    let fine = (step / 8).max(1);
    let mut width = near.saturating_sub(step).max(narrowest);
    while width <= (near + step).min(max_side) {
        try_width(&mut best, width);
        width += fine;
    }
    best
}

/// The packing in a bin of the given width and the least height that works.
fn fit_width(
    sizes: &[(u32, u32)],
    area: u64,
    tallest: u32,
    width: u32,
    max_side: u32,
) -> Option<Packing> {
    let mut best = try_pack(sizes, width, max_side)?;
    let mut low = tallest.max(area.div_ceil(u64::from(width)) as u32);
    let mut high = max_side;
    while low < high {
        let middle = low + (high - low) / 2;
        match try_pack(sizes, width, middle) {
            Some(found) => {
                best = found;
                high = middle;
            }
            None => low = middle + 1,
        }
    }
    Some(best)
}

fn try_pack(sizes: &[(u32, u32)], width: u32, height: u32) -> Option<Packing> {
    let mut rects = GroupedRectsToPlace::<usize, ()>::new();
    for (i, &(w, h)) in sizes.iter().enumerate() {
        rects.push_rect(i, None, RectToInsert::new(w, h, 1));
    }
    let mut bins = BTreeMap::from([(0u8, TargetBin::new(width, height, 1))]);
    let packed = pack_rects(&rects, &mut bins, &volume_heuristic, &contains_smallest_box).ok()?;

    let mut placements = vec![Placement { x: 0, y: 0 }; sizes.len()];
    let (mut used_width, mut used_height) = (0, 0);
    for (&i, (_, location)) in packed.packed_locations() {
        placements[i] = Placement {
            x: location.x(),
            y: location.y(),
        };
        used_width = used_width.max(location.x() + location.width());
        used_height = used_height.max(location.y() + location.height());
    }
    Some(((used_width, used_height), placements))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn overlaps(a: (Placement, (u32, u32)), b: (Placement, (u32, u32))) -> bool {
        a.0.x < b.0.x + b.1.0
            && b.0.x < a.0.x + a.1.0
            && a.0.y < b.0.y + b.1.1
            && b.0.y < a.0.y + a.1.1
    }

    fn check(sizes: &[(u32, u32)], max_side: u32) -> (u32, u32) {
        let ((width, height), placements) = pack(sizes, max_side).unwrap();
        assert_eq!(placements.len(), sizes.len());
        for (i, (p, &(w, h))) in placements.iter().zip(sizes).enumerate() {
            assert!(p.x + w <= width && p.y + h <= height, "{i} sticks out");
            for j in 0..i {
                assert!(
                    !overlaps((*p, (w, h)), (placements[j], sizes[j])),
                    "{i} overlaps {j}"
                );
            }
        }
        (width, height)
    }

    #[test]
    fn the_game_sprites_pack_tightly() {
        // 14 sprites of 64x64 with a one texel gutter. A 4 by 4 grid is
        // 264x264; a bin of any shape can do better.
        let (width, height) = check(&[(66, 66); 14], 2048);
        assert!(width * height <= 264 * 264, "{width}x{height}");
        assert!(width * height >= 14 * 66 * 66);
    }

    #[test]
    fn a_bin_can_be_a_rectangle() {
        // Five strips 100 wide and 10 tall stack into 100x50. A square bin
        // would have to be 100x100.
        assert_eq!(check(&[(100, 10); 5], 2048), (100, 50));
    }

    #[test]
    fn mixed_sizes_do_not_overlap() {
        let sizes: Vec<(u32, u32)> = (0..60).map(|i| (8 + i % 17 * 3, 5 + i % 11 * 4)).collect();
        check(&sizes, 2048);
    }

    #[test]
    fn nothing_to_pack_is_empty() {
        assert_eq!(pack(&[], 64), Some(((0, 0), Vec::new())));
    }

    #[test]
    fn too_many_rectangles_do_not_fit() {
        assert!(pack(&[(40, 40); 4], 64).is_none());
        assert!(pack(&[(65, 1)], 64).is_none());
    }
}
