//! Stem darkening for small text. FreeType thickens vertical stems at small
//! sizes to make up for coverage lost to gamma-unaware blending, and the
//! baked glyphs must look the same as the ones it drew.
//!
//! The curve and the embolden follow FreeType's autofitter. Its output on
//! Inter at 12 to 27 px grows stems by about half of what that code computes,
//! in proportion to the size, and the cause is unknown. `FIT` scales the
//! computed amount to the measured growth. Remove it if a FreeType source
//! check ever finds the cause.
use zeno::{Command, Point};

pub fn mul_fix(a: i64, b: i64) -> i64 {
    let s = if (a < 0) != (b < 0) { -1 } else { 1 };
    s * ((a.abs() * b.abs() + 0x8000) >> 16)
}
pub fn div_fix(a: i64, b: i64) -> i64 {
    let s = if (a < 0) != (b < 0) { -1 } else { 1 };
    s * (((a.abs() << 16) + b.abs() / 2) / b.abs())
}
fn mul_div(a: i64, b: i64, c: i64) -> i64 {
    let s = if ((a < 0) ^ (b < 0)) ^ (c < 0) { -1 } else { 1 };
    s * ((a.abs() * b.abs() + c.abs() / 2) / c.abs())
}
fn msb(x: i64) -> i64 {
    63 - (x as u64).leading_zeros() as i64
}
fn fixed(i: i64) -> i64 {
    i << 16
}

const PARAMS: [i64; 8] = [500, 400, 1000, 275, 1667, 275, 2333, 0];

/// Font units to embolden by, 16.16.
fn compute(ppem_px: i64, upem: i64, standard_width: i64) -> i64 {
    let ppem = fixed(4).max(fixed(ppem_px));
    let em_ratio = div_fix(fixed(1000), fixed(upem));
    let [x1, y1, x2, y2, x3, y3, x4, y4] = PARAMS;
    let stem_per_1000 = if standard_width <= 0 {
        fixed(75)
    } else {
        mul_fix(fixed(standard_width), em_ratio)
    };
    let log2 = msb(stem_per_1000) + msb(ppem);
    let scaled = if log2 >= 46 {
        fixed(x4)
    } else {
        mul_fix(stem_per_1000, ppem)
    };
    let seg = |xa: i64, ya: i64, xb: i64, yb: i64| {
        let x = stem_per_1000 - div_fix(fixed(xa), ppem);
        mul_div(x, yb - ya, xb - xa) + div_fix(fixed(ya), ppem)
    };
    let amount = if scaled < fixed(x1) {
        div_fix(fixed(y1), ppem)
    } else if scaled < fixed(x2) {
        seg(x1, y1, x2, y2)
    } else if scaled < fixed(x3) {
        seg(x2, y2, x3, y3)
    } else if scaled < fixed(x4) {
        seg(x3, y3, x4, y4)
    } else {
        div_fix(fixed(y4), ppem)
    };
    div_fix(amount, em_ratio)
}

/// Divisor of the size in the measured correction. Measured on Inter against
/// FreeType 2.15: stems grow by 7.3/64 px at 12 px, 10.5/64 px at 18 px.
const FIT: f64 = 29.2;

/// The x darkening for one size, in 26.6 pixels.
pub fn amount_x(ppem: u32, upem: u32, standard_width: i64) -> i64 {
    let x_scale = div_fix(i64::from(ppem) << 6, i64::from(upem));
    let units = compute(i64::from(ppem), i64::from(upem), standard_width);
    let computed = (mul_fix(units, x_scale) + 0x8000) >> 16;
    (computed as f64 * f64::from(ppem) / FIT).round() as i64
}

fn norm_len(v: &mut (i64, i64)) -> i64 {
    let len = ((v.0 * v.0 + v.1 * v.1) as f64).sqrt();
    if len == 0.0 {
        return 0;
    }
    *v = (
        (v.0 as f64 / len * 65536.0).round() as i64,
        (v.1 as f64 / len * 65536.0).round() as i64,
    );
    len.round() as i64
}

/// Moves the points of one contour (26.6) outward in x, as FreeType's outline
/// embolden does with a zero y strength.
fn embolden_contour(pts: &mut [(i64, i64)], xs: i64, truetype: bool) {
    let (first, last) = (0usize, pts.len() - 1);
    let next = |j: usize| if j < last { j + 1 } else { first };
    let (mut i, mut j, mut k) = (last, first, -1isize);
    let (mut l_in, mut in_, mut anchor, mut l_anchor) = (0i64, (0i64, 0i64), (0i64, 0i64), 0i64);
    while j != i && i as isize != k {
        let (out, l_out);
        if j as isize != k {
            let mut o = (pts[j].0 - pts[i].0, pts[j].1 - pts[i].1);
            let l = norm_len(&mut o);
            if l == 0 {
                j = next(j);
                continue;
            }
            out = o;
            l_out = l;
        } else {
            out = anchor;
            l_out = l_anchor;
        }
        if l_in != 0 {
            if k < 0 {
                k = i as isize;
                anchor = in_;
                l_anchor = l_in;
            }
            let mut d = mul_fix(in_.0, out.0) + mul_fix(in_.1, out.1);
            let mut shift = (0i64, 0i64);
            if d > -0xF000 {
                d += 0x10000;
                shift = (in_.1 + out.1, in_.0 + out.0);
                if truetype {
                    shift.0 = -shift.0;
                } else {
                    shift.1 = -shift.1;
                }
                let mut q = mul_fix(out.0, in_.1) - mul_fix(out.1, in_.0);
                if truetype {
                    q = -q;
                }
                let l = l_in.min(l_out);
                shift.0 = if mul_fix(xs, q) <= mul_fix(l, d) {
                    mul_div(shift.0, xs, d)
                } else {
                    mul_div(shift.0, l, q)
                };
                // The y strength is zero: shift.y = mul_div(shift.y, 0, d) = 0.
                shift.1 = 0;
            }
            while i != j {
                pts[i].0 += xs + shift.0;
                i = next(i);
            }
        } else {
            i = j;
        }
        in_ = out;
        l_in = l_out;
        j = next(j);
    }
}

/// Emboldens the outline in x by `strength` (26.6). FreeType does this before
/// hinting; light hinting does not move points in x, so doing it after is the
/// same.
pub fn embolden_x(cmds: &[Command], strength: i64) -> Vec<Command> {
    let xs = strength / 2;
    if xs == 0 {
        return cmds.to_vec();
    }
    // Flatten to contours of 26.6 points, remembering the shape.
    let to = |p: Point| ((p.x * 64.0).round() as i64, (p.y * 64.0).round() as i64);
    let mut contours: Vec<Vec<(i64, i64)>> = Vec::new();
    for c in cmds {
        match *c {
            Command::MoveTo(p) => contours.push(vec![to(p)]),
            Command::LineTo(p) => contours.last_mut().unwrap().push(to(p)),
            Command::QuadTo(a, b) => contours.last_mut().unwrap().extend([to(a), to(b)]),
            Command::CurveTo(a, b, c) => contours.last_mut().unwrap().extend([to(a), to(b), to(c)]),
            Command::Close => {}
        }
    }
    let mut area = 0i64;
    for c in &contours {
        let mut prev = *c.last().unwrap();
        for &cur in c {
            area += (cur.1 - prev.1) * (cur.0 + prev.0);
            prev = cur;
        }
    }
    if area == 0 {
        return cmds.to_vec();
    }
    for c in &mut contours {
        embolden_contour(c, xs, area < 0);
    }
    let back = |p: (i64, i64)| Point::new(p.0 as f32 / 64.0, p.1 as f32 / 64.0);
    let mut out = Vec::with_capacity(cmds.len());
    let (mut ci, mut pi) = (0usize, 0usize);
    let take = |ci: usize, pi: &mut usize| {
        let p = back(contours[ci][*pi]);
        *pi += 1;
        p
    };
    for c in cmds {
        out.push(match *c {
            Command::MoveTo(_) => {
                pi = 0;
                Command::MoveTo(take(ci, &mut pi))
            }
            Command::LineTo(_) => Command::LineTo(take(ci, &mut pi)),
            Command::QuadTo(..) => {
                let (a, b) = (take(ci, &mut pi), take(ci, &mut pi));
                Command::QuadTo(a, b)
            }
            Command::CurveTo(..) => {
                let (a, b, c) = (take(ci, &mut pi), take(ci, &mut pi), take(ci, &mut pi));
                Command::CurveTo(a, b, c)
            }
            Command::Close => {
                ci += 1;
                Command::Close
            }
        });
    }
    out
}

/// Width of the vertical stems of "o" in font units: what autofit uses as the
/// standard width. Measured as the stroke thickness at mid height.
pub fn standard_width(outline: &[Command]) -> i64 {
    let mut ys = (f32::MAX, f32::MIN);
    for c in outline {
        let ps: Vec<Point> = match *c {
            Command::MoveTo(p) | Command::LineTo(p) => vec![p],
            Command::QuadTo(a, b) => vec![a, b],
            Command::CurveTo(a, b, c) => vec![a, b, c],
            Command::Close => vec![],
        };
        for p in ps {
            ys = (ys.0.min(p.y), ys.1.max(p.y));
        }
    }
    let mid = (ys.0 + ys.1) / 2.0;
    let mut xs = Vec::new();
    let mut cur = Point::new(0.0, 0.0);
    let mut start = cur;
    let mut cross = |a: Point, b: Point| {
        if (a.y - mid) * (b.y - mid) < 0.0 {
            xs.push(a.x + (mid - a.y) / (b.y - a.y) * (b.x - a.x));
        }
    };
    for c in outline {
        match *c {
            Command::MoveTo(p) => {
                cur = p;
                start = p;
            }
            Command::LineTo(p) => {
                cross(cur, p);
                cur = p;
            }
            Command::QuadTo(a, b) => {
                let mut prev = cur;
                for s in 1..=32 {
                    let t = s as f32 / 32.0;
                    let u = 1.0 - t;
                    let p = Point::new(
                        u * u * cur.x + 2.0 * u * t * a.x + t * t * b.x,
                        u * u * cur.y + 2.0 * u * t * a.y + t * t * b.y,
                    );
                    cross(prev, p);
                    prev = p;
                }
                cur = b;
            }
            Command::CurveTo(..) => {}
            Command::Close => {
                cross(cur, start);
                cur = start;
            }
        }
    }
    xs.sort_by(f32::total_cmp);
    if xs.len() < 4 {
        return 0;
    }
    (((xs[1] - xs[0]) + (xs[3] - xs[2])) / 2.0).round() as i64
}
