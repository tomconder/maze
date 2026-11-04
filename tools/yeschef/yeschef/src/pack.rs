//! Rectangle packing for the sprite atlas and the font atlas.

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

/// Packs the `(width, height)` rectangles into the smallest square bin that
/// fits them, of at most `max_side`. Returns the bounding size of what was
/// placed and a placement for each input rectangle. Returns `None` when they
/// do not fit. Rectangles never rotate, because sprites and glyphs are sampled
/// axis aligned.
///
/// The bin side is found by bisection, which assumes that a bin that fits
/// the rectangles stays big enough as it grows. The packer is greedy, so that
/// can fail in theory, and the result is then a little bigger than the best
/// one.
pub fn pack(sizes: &[(u32, u32)], max_side: u32) -> Option<((u32, u32), Vec<Placement>)> {
    if sizes.is_empty() {
        return Some(((0, 0), Vec::new()));
    }
    let area: u64 = sizes
        .iter()
        .map(|&(w, h)| u64::from(w) * u64::from(h))
        .sum();
    let widest = sizes.iter().map(|&(w, _)| w).max().unwrap();
    let tallest = sizes.iter().map(|&(_, h)| h).max().unwrap();

    let mut low = (area as f64).sqrt().ceil() as u32;
    low = low.max(widest).max(tallest);
    let mut high = max_side;
    let mut best = try_pack(sizes, high)?;
    // The smallest side that fits is in low..=high, and `best` fits at high.
    while low < high {
        let middle = low + (high - low) / 2;
        match try_pack(sizes, middle) {
            Some(found) => {
                best = found;
                high = middle;
            }
            None => low = middle + 1,
        }
    }
    Some(best)
}

fn try_pack(sizes: &[(u32, u32)], side: u32) -> Option<((u32, u32), Vec<Placement>)> {
    let mut rects = GroupedRectsToPlace::<usize, ()>::new();
    for (i, &(w, h)) in sizes.iter().enumerate() {
        rects.push_rect(i, None, RectToInsert::new(w, h, 1));
    }
    let mut bins = BTreeMap::from([(0u8, TargetBin::new(side, side, 1))]);
    let packed = pack_rects(&rects, &mut bins, &volume_heuristic, &contains_smallest_box).ok()?;

    let mut placements = vec![Placement { x: 0, y: 0 }; sizes.len()];
    let (mut width, mut height) = (0, 0);
    for (&i, (_, location)) in packed.packed_locations() {
        placements[i] = Placement {
            x: location.x(),
            y: location.y(),
        };
        width = width.max(location.x() + location.width());
        height = height.max(location.y() + location.height());
    }
    Some(((width, height), placements))
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
    fn the_game_sprites_pack_without_waste() {
        // 14 sprites of 64x64 with a one texel gutter.
        let size = check(&[(66, 66); 14], 2048);
        assert_eq!(size, (264, 264), "four columns and four rows");
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
