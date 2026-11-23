//! Drawing the panel with GoodLookin DRI, in panel units.

use std::hash::{Hash, Hasher};

use openplugin::dri::goodlookindri::color::Color;
use openplugin::dri::goodlookindri::draw::{self, KnobMode};
use std::sync::Arc;

use openplugin::dri::goodlookindri::scene::{Cmd, Image, Paint, Point, Rect, Scene, Shape};
use openplugin::dri::goodlookindri::{hardware, meters};
use goodlookindri_shell::Framebuffer;
use openplugin::dri::{Control, ControlSet};

use super::graphs::{self, CompCurve, EqGraph};
use super::lettering;
use super::panel::*;
use super::text;
use crate::params::index as ix;
use crate::{EditorSettings, HqStatus, COUNT, PARAMS};

/// What the meters show this frame.
#[derive(Clone, Copy, Debug)]
pub struct Meters {
    /// The VU needle's position, 0..1.
    pub needle: f32,
    /// The VU reading.
    pub vu: f32,
    /// Peaks in dBFS.
    pub input_db: f32,
    pub output_db: f32,
    /// dB of reduction.
    pub comp_gr: f32,
    pub gate_gr: f32,
    /// 0..1.
    pub deess: f32,
    /// 1 when a stage clipped, falling to 0 over 20 ticks.
    pub overload: f32,
    pub key_input: bool,
    /// What the numeric readouts show: the highest of each reading over the last
    /// `READ_SECONDS`, changed only that often so a number can be read.
    pub shown: Reading,
    /// The highest readings since `shown` last changed, and for how long.
    held: Reading,
    held_for: f32,
}

/// How long a numeric readout keeps a value.
pub const READ_SECONDS: f32 = 1.5;

/// The readings the Digital skin prints as numbers.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Reading {
    pub input_db: f32,
    pub output_db: f32,
    pub comp_gr: f32,
    pub gate_gr: f32,
    pub vu: f32,
}

impl Reading {
    const SILENT: Self = Self { input_db: -180.0, output_db: -180.0, comp_gr: 0.0, gate_gr: 0.0, vu: -100.0 };

    fn highest(self, o: Self) -> Self {
        Self { input_db: self.input_db.max(o.input_db), output_db: self.output_db.max(o.output_db), comp_gr: self.comp_gr.max(o.comp_gr), gate_gr: self.gate_gr.max(o.gate_gr), vu: self.vu.max(o.vu) }
    }
}

impl Default for Meters {
    fn default() -> Self {
        Self {
            needle: 0.0,
            vu: -100.0,
            input_db: -180.0,
            output_db: -180.0,
            comp_gr: 0.0,
            gate_gr: 0.0,
            deess: 0.0,
            overload: 0.0,
            key_input: false,
            shown: Reading::SILENT,
            held: Reading::SILENT,
            // The first reading shows at once.
            held_for: READ_SECONDS,
        }
    }
}

impl Meters {
    /// Take the current readings into the numbers shown, `dt` seconds after the last time.
    pub fn read(&mut self, dt: f32) {
        let now = Reading { input_db: self.input_db, output_db: self.output_db, comp_gr: self.comp_gr, gate_gr: self.gate_gr, vu: self.vu };
        self.held = self.held.highest(now);
        self.held_for += dt.max(0.0);
        if self.held_for >= READ_SECONDS {
            self.shown = self.held;
            self.held = Reading::SILENT;
            self.held_for = 0.0;
        }
    }
}

/// Everything a widget's look depends on.
pub struct Looks<'a> {
    pub values: &'a [f32; COUNT],
    pub controls: &'a ControlSet,
    /// The control of each widget that has one.
    pub ctl: &'a [Option<usize>],
    /// The widget (and the part of it) under the pointer, and the one held.
    pub hot: Option<(usize, usize)>,
    pub down: Option<(usize, usize)>,
    pub meters: &'a Meters,
    pub settings: EditorSettings,
    pub hq: HqStatus,
    pub page: Page,
    pub keys_active: bool,
    pub sample_rate: f32,
    pub eq: &'a EqGraph,
    pub vu: &'a VuParts,
}

/// GoodLookin DRI 0.2's lit tier: each knob is shaded per pixel once and kept as a sprite.
const KNOBS: KnobMode = KnobMode::Lit;

const fn c(argb: u32) -> Color {
    Color(argb)
}

fn alpha(argb: u32, a: f32) -> Color {
    Color(argb).with_alpha(a)
}

fn rect(r: R) -> Rect {
    Rect::new(r.0, r.1, r.2, r.3)
}

fn inside(r: R, x: f32, y: f32) -> bool {
    x >= r.0 && x < r.0 + r.2 && y >= r.1 && y < r.1 + r.3
}

fn text_c(s: &mut Scene, x: f32, y: f32, t: &str, colour: Color, size: f32) {
    lettering::draw(s, x - lettering::width(t, size, false) * 0.5, y, t, colour, size, false);
}

fn text_l(s: &mut Scene, x: f32, y: f32, t: &str, colour: Color, size: f32) {
    lettering::draw(s, x, y, t, colour, size, false);
}

fn text_r(s: &mut Scene, right: f32, y: f32, t: &str, colour: Color, size: f32) {
    lettering::draw(s, right - lettering::width(t, size, false), y, t, colour, size, false);
}

/// The analogue panel's legends are printed bold, as on the hardware and in the release.
fn heavy(s: &mut Scene, left: f32, y: f32, t: &str, colour: Color, size: f32) {
    lettering::draw(s, left, y, t, colour, size, true);
}

fn heavy_c(s: &mut Scene, x: f32, y: f32, t: &str, colour: Color, size: f32) {
    heavy(s, x - lettering::width(t, size, true) * 0.5, y, t, colour, size);
}

fn heavy_r(s: &mut Scene, right: f32, y: f32, t: &str, colour: Color, size: f32) {
    heavy(s, right - lettering::width(t, size, true), y, t, colour, size);
}

/// The rectangle a widget's drawing can change: itself, and what it draws around itself
/// (an LED's glow, a lamp's bloom, the focus ring, the VU's shadow). Kept tight: a widget
/// is repainted whenever a neighbour's rectangle touches its own.
pub fn reach(w: &Widget) -> R {
    let (x, y, width, height) = w.rect;
    match w.kind {
        Kind::Knob { .. } => {
            // A lit knob is one picture with its soft shadow on the panel around it: what
            // of it is not clear reaches 1.62 radii from the centre (measured at 0.5 to 2
            // pixels per unit). Then the focus ring, and the value plate on the legend line.
            let (cx, cy, radius) = knob_circle(w.rect);
            let far = 1.7 * radius + 1.0;
            let ly = y + height - KNOB_LEGEND * 0.5;
            let (x0, y0) = ((cx - far).min(x - 5.0).min(cx - PLATE_W * 0.5 - 1.0), (cy - far).min(y - 5.0));
            let (x1, y1) = ((cx + far).max(x + width + 5.0).max(cx + PLATE_W * 0.5 + 1.0), (cy + far).max(ly + PLATE_H * 0.5 + 1.0));
            (x0, y0, x1 - x0, y1 - y0)
        }
        // Only the column of LEDs changes, each with a halo when lit; the scale is printed.
        Kind::Bar { .. } => {
            let column = bar_column(w.rect);
            (column.0 - 2.0, column.1 - 2.0, column.2 + 4.0, column.3 + 4.0)
        }
        // A lamp of radius r spills light to 3.6 r from its centre: 11 units here, around
        // lamps 7 units under the top edge.
        Kind::GateLeds => (x - 1.0, y - 5.0, width + 2.0, height + 5.0),
        _ => {
            let m = match w.kind {
                Kind::EqGraph | Kind::CompGraph | Kind::Readouts { .. } | Kind::Label { .. } | Kind::Status { .. } | Kind::VuText => 1.0,
                Kind::Vu => VU_MARGIN,
                Kind::Hero { .. } | Kind::Slider { .. } | Kind::Toggle { .. } | Kind::Seg { .. } | Kind::PropSeg { .. } | Kind::Button { .. } => 5.0,
                _ => 7.0,
            };
            (x - m, y - m, width + 2.0 * m, height + 2.0 * m)
        }
    }
}

// ---- the VU meter ----

/// How far the meter's shadow reaches out of its rectangle.
const VU_MARGIN: f32 = 10.0;
const VU_RECT: R = VU;

/// GoodLookin draws the meter in one call: the case and face, then the needle, then the
/// glass over it. The face and the glass never change and are most of the cost, so they
/// are split off: the face goes into the panel's static layer and the glass is kept as an
/// image, leaving the needle as the only thing drawn per frame.
pub struct VuParts {
    /// How many commands come before the needle's, and how many after.
    before: usize,
    after: usize,
    /// The glass at one device scale: (scale bits, where it goes, the picture).
    glass: Option<(u32, Rect, Arc<Image>)>,
}

impl Default for VuParts {
    fn default() -> Self {
        // The needle's commands are the ones that differ between two positions.
        let (a, b) = (Self::commands(0.25), Self::commands(0.7));
        let same = |x: &Cmd, y: &Cmd| x == y;
        let before = a.iter().zip(&b).take_while(|(x, y)| same(x, y)).count();
        let after = if a.len() == b.len() { a.iter().rev().zip(b.iter().rev()).take_while(|(x, y)| same(x, y)).count().min(a.len() - before) } else { 0 };
        Self { before, after, glass: None }
    }
}

impl VuParts {
    fn commands(needle: f32) -> Vec<Cmd> {
        let mut scene = Scene::new();
        meters::draw_vu_meter(&mut scene, rect(VU_RECT), needle, meters::VuFace::Cream);
        scene.cmds
    }

    /// The case and the face, for the static layer.
    fn face(&self) -> Vec<Cmd> {
        let mut all = Self::commands(0.0);
        all.truncate(self.before);
        all
    }

    /// The needle alone.
    fn needle(&self, position: f32) -> Vec<Cmd> {
        let all = Self::commands(position);
        let end = all.len() - self.after;
        all[self.before.min(end)..end].to_vec()
    }

    /// Make the picture of the glass for `scale` device pixels per unit, if it is not there.
    /// It is drawn once over black and once over white, which gives each pixel's colour and
    /// how much of what is under it shows through.
    pub fn prepare(&mut self, scale: f32) {
        let scale = if scale.is_finite() { scale.clamp(0.25, 8.0) } else { 2.0 };
        if self.after == 0 || self.glass.as_ref().is_some_and(|g| g.0 == scale.to_bits()) {
            return;
        }
        let all = Self::commands(0.0);
        let mut scene = Scene::new();
        scene.cmds = all[all.len() - self.after..].to_vec();
        let (x, y, w, h) = VU_RECT;
        let x0 = ((x - VU_MARGIN) * scale).floor().max(0.0) as usize;
        let y0 = ((y - VU_MARGIN) * scale).floor().max(0.0) as usize;
        let x1 = ((x + w + VU_MARGIN) * scale).ceil() as usize;
        let y1 = ((y + h + VU_MARGIN) * scale).ceil() as usize;
        let render = |background: u32| {
            let mut frame = Framebuffer::new(x1, y1);
            frame.clear(Color(background));
            frame.replay(&scene, scale);
            frame
        };
        let (black, white) = (render(0xff000000), render(0xffffffff));
        let mut pixels = Vec::with_capacity((x1 - x0) * (y1 - y0));
        for row in y0..y1 {
            for col in x0..x1 {
                let (b, w) = (black.pixels[row * x1 + col], white.pixels[row * x1 + col]);
                let channel = |shift: u32| (((b >> shift) & 255) as i32, ((w >> shift) & 255) as i32);
                let cover = |(b, w): (i32, i32)| 255 - (w - b).clamp(0, 255);
                let alpha = (cover(channel(16)) + cover(channel(8)) + cover(channel(0))) / 3;
                let straight = |(b, _): (i32, i32)| if alpha > 0 { (b * 255 / alpha).clamp(0, 255) as u32 } else { 0 };
                pixels.push(((alpha as u32) << 24) | (straight(channel(16)) << 16) | (straight(channel(8)) << 8) | straight(channel(0)));
            }
        }
        let image = Image::from_straight((x1 - x0) as u32, (y1 - y0) as u32, &pixels);
        let place = Rect::new(x0 as f32 / scale, y0 as f32 / scale, (x1 - x0) as f32 / scale, (y1 - y0) as f32 / scale);
        self.glass = Some((scale.to_bits(), place, Arc::new(image)));
    }

    /// The needle and the glass over it.
    fn draw(&self, s: &mut Scene, position: f32) {
        s.cmds.extend(self.needle(position));
        match &self.glass {
            Some((_, place, image)) => s.image(image, *place, 1.0),
            // Not prepared (a caller that skipped `prepare`): the glass as drawing commands.
            None => {
                let all = Self::commands(position);
                s.cmds.extend(all[all.len() - self.after..].iter().cloned());
            }
        }
    }
}

// ---- panel units to points ----

/// Put a scene drawn in panel units into the window: `k` points per unit, the panel's top
/// left corner at (`dx`, `dy`) points.
pub fn place(scene: &mut Scene, k: f32, dx: f32, dy: f32) {
    if k == 1.0 && dx == 0.0 && dy == 0.0 {
        return;
    }
    let p = |p: &mut Point| *p = Point::new(p.x * k + dx, p.y * k + dy);
    let r = |r: &mut Rect| *r = Rect::new(r.x * k + dx, r.y * k + dy, r.w * k, r.h * k);
    let shape = |s: &mut Shape| match s {
        Shape::Ellipse(rect) => r(rect),
        Shape::Round(rect, radius) => {
            r(rect);
            *radius *= k;
        }
    };
    for cmd in &mut scene.cmds {
        match cmd {
            Cmd::FillEllipse { rect, .. } | Cmd::FillRect { rect, .. } => r(rect),
            Cmd::StrokeEllipse { rect, width, .. } => {
                r(rect);
                *width *= k;
            }
            Cmd::FillRoundRect { rect, radius, .. } | Cmd::ClipRound { rect, radius } => {
                r(rect);
                *radius *= k;
            }
            Cmd::StrokeRoundRect { rect, radius, width, .. } => {
                r(rect);
                *radius *= k;
                *width *= k;
            }
            Cmd::Line { a, b, width, .. } => {
                p(a);
                p(b);
                *width *= k;
            }
            Cmd::FillPoly { pts, .. } => pts.iter_mut().for_each(p),
            Cmd::StrokePoly { pts, width, .. } => {
                pts.iter_mut().for_each(p);
                *width *= k;
            }
            Cmd::StrokeArc { center, radius, width, .. } => {
                p(center);
                *radius *= k;
                *width *= k;
            }
            Cmd::Text { at, size, .. } => {
                p(at);
                *size *= k;
            }
            Cmd::Fill { shape: s, paint, feather, .. } => {
                shape(s);
                *feather *= k;
                match paint {
                    Paint::Radial { center, rx, ry, focus, .. } => {
                        p(center);
                        p(focus);
                        *rx *= k;
                        *ry *= k;
                    }
                    Paint::Conic { center, .. } => p(center),
                    Paint::Solid(_) | Paint::Linear { .. } => {}
                }
            }
            Cmd::Shadow { shape: s, sigma, .. } => {
                shape(s);
                *sigma *= k;
            }
            Cmd::InnerShadow { shape: s, offset, sigma, .. } => {
                shape(s);
                // A distance, not a place.
                *offset = Point::new(offset.x * k, offset.y * k);
                *sigma *= k;
            }
            Cmd::Image { dest, .. } => r(dest),
            _ => {}
        }
    }
}

// ---- geometry shared by drawing and hit testing ----

/// A knob widget of the analogue panel from the top, as in the release (Widgets.h): its
/// caption, the knob, and the line of end legends.
const KNOB_CAPTION: f32 = 12.0;
const KNOB_LEGEND: f32 = 11.0;
/// How far a lit knob with its printed scale reaches from its centre, in radii: the dots
/// sit at 1.12, and the body leans a little up the screen under the tilted camera. The
/// classic knob filled its space to the edge; the lit one is drawn this much smaller so
/// that it clears the caption above, the legends below and its neighbours.
const KNOB_REACH_X: f32 = 1.14;
const KNOB_REACH_Y: f32 = 1.15;

/// The value shown over the legend line while a knob is held, pointed at or focused: the
/// largest plate, and the text in it.
const PLATE_W: f32 = 84.0;
const PLATE_H: f32 = 19.0;
const PLATE_TEXT: f32 = 14.0;

/// The knob's circle inside a knob widget: (centre x, centre y, radius). The centre is
/// where the release drew it.
pub fn knob_circle(r: R) -> (f32, f32, f32) {
    let h = r.3 - KNOB_CAPTION - KNOB_LEGEND;
    let radius = (r.2 * 0.5 / KNOB_REACH_X).min(h * 0.5 / KNOB_REACH_Y);
    (r.0 + r.2 * 0.5, r.1 + KNOB_CAPTION + h * 0.5, radius)
}

/// The LEDs of a level bar: a column 9 units wide at the right of its rectangle.
pub fn bar_column(r: R) -> R {
    (r.0 + r.2 - 9.0, r.1, 9.0, r.3)
}

/// The value line under the large rotary of the overview.
const HERO_VALUE: f32 = 26.0;

pub fn hero_circle(r: R) -> (f32, f32, f32) {
    let h = r.3 - HERO_VALUE;
    (r.0 + r.2 * 0.5, r.1 + h * 0.5, (r.2 * 0.5 / KNOB_REACH_X).min(h * 0.5 / KNOB_REACH_Y))
}

/// The key of a step selector, and its lamp rows.
pub fn step_key(r: R, key_w: f32) -> R {
    (r.0, r.1 + (r.3 - KEY_H) * 0.5, key_w, KEY_H)
}

pub fn step_rows(r: R, key_w: f32, n: usize) -> R {
    let h = (r.3 / n.max(1) as f32).floor().max(14.0);
    (r.0 + key_w + 2.0, r.1, r.2 - key_w - 2.0, h * n as f32)
}

/// The key of a selector of an editor setting is this wide.
const PROP_KEY: f32 = 46.0;

pub fn prop_rows(r: R, n: usize) -> R {
    let h = (r.3 / n.max(1) as f32).floor().min(13.0);
    (r.0 + PROP_KEY + 2.0, r.1, r.2 - PROP_KEY - 2.0, h * n as f32)
}

/// The columns of a slider: its name, and its value after the track.
const SLIDER_NAME: f32 = 68.0;
const SLIDER_VALUE: f32 = 66.0;

/// A slider's track: (x0, x1, centre y).
pub fn slider_track(r: R) -> (f32, f32, f32) {
    (r.0 + SLIDER_NAME + 8.0, r.0 + r.2 - SLIDER_VALUE - 6.0, r.1 + r.3 * 0.5)
}

/// The name column of a row of segments.
const SEG_NAME: f32 = 76.0;

/// The segments of a `Seg` widget.
pub fn seg_rect(r: R) -> R {
    (r.0 + SEG_NAME, r.1, r.2 - SEG_NAME, r.3)
}

/// Which of `n` equal parts of `r` along x holds `x`.
pub fn part(r: R, n: usize, x: f32) -> usize {
    (((x - r.0) / r.2 * n as f32).floor().max(0.0) as usize).min(n - 1)
}

fn row(r: R, n: usize, y: f32) -> usize {
    (((y - r.1) / r.3 * n as f32).floor().max(0.0) as usize).min(n - 1)
}

/// The part of a widget that answers the mouse itself (not through the
/// control set), if the point is on one: a route key, a selector's key or
/// lamp row, a segment, a button, a graph (its handle, or `usize::MAX`).
pub fn custom_hit(w: &Widget, x: f32, y: f32, looks: &Looks) -> Option<usize> {
    if !inside(w.rect, x, y) {
        return None;
    }
    match &w.kind {
        Kind::Route { .. } => Some(part(w.rect, 3, x)),
        Kind::Step { key_w, names, hq, .. } => {
            let rows = step_rows(w.rect, *key_w, names.len());
            if inside(step_key(w.rect, *key_w), x, y) {
                Some(0)
            } else if *hq && inside(rows, x, y) {
                Some(1 + row(rows, names.len(), y))
            } else {
                None
            }
        }
        Kind::PropStep { names, .. } => {
            let rows = prop_rows(w.rect, names.len());
            if x < w.rect.0 + PROP_KEY {
                Some(0)
            } else if inside(rows, x, y) {
                Some(1 + row(rows, names.len(), y))
            } else {
                None
            }
        }
        Kind::SkinKey | Kind::Button { .. } => Some(0),
        Kind::Seg { hq: true, names, .. } => inside(seg_rect(w.rect), x, y).then(|| part(seg_rect(w.rect), names.len(), x)),
        Kind::PropSeg { names, .. } => Some(part(w.rect, names.len(), x)),
        Kind::EqGraph => Some(looks.eq.pick(&graphs::eq_plot(w.rect), x, y).unwrap_or(usize::MAX)),
        Kind::CompGraph => Some(CompCurve::new(looks.values).pick(&graphs::comp_plot(w.rect), x, y).unwrap_or(usize::MAX)),
        _ => None,
    }
}

// ---- what each control says ----

/// The end legends of a knob, which follow the range switches.
fn legends(param: usize, lo: &'static str, hi: &'static str, v: &[f32; COUNT]) -> (&'static str, &'static str) {
    match param {
        // The instrument input's trim runs higher.
        ix::TRIM if v[ix::SOURCE].round() == 2.0 => ("+7", "+40"),
        ix::LMFFREQ if v[ix::LMFX3] > 0.5 => ("120", "1.2k"),
        ix::LMFFREQ => ("40", "400"),
        ix::HMFFREQ if v[ix::HMFX3] > 0.5 => ("1.8k", "18k"),
        ix::HMFFREQ => ("600", "6k"),
        _ => (lo, hi),
    }
}

/// The legend of a knob that has no end legends: the position it is switched to.
fn extra_legend(param: usize, v: &[f32; COUNT]) -> Option<&'static str> {
    match param {
        ix::LFFREQ => Some(text::low_shelf(v[ix::LFFREQ].round() as usize, v[ix::LFHIGH] > 0.5)),
        ix::HFFREQ => Some(text::high_shelf(v[ix::HFFREQ].round() as usize, v[ix::HFHIGH] > 0.5)),
        _ => None,
    }
}

/// Whether the filters do anything: in the audio path with the equaliser on, or feeding a key.
fn filters_active(v: &[f32; COUNT]) -> bool {
    let route = v[ix::FILTROUTE].round();
    route >= 2.0 || (route == 1.0 && v[ix::ALLEQ] > 0.5 && v[ix::BYPASS] < 0.5)
}

/// The option a step selector or a row of segments shows as chosen.
fn selected(param: usize, hq: bool, v: &[f32; COUNT], n: usize) -> usize {
    if hq && v[ix::HQ] > 0.5 {
        n - 1
    } else {
        (v[param].round().max(0.0) as usize).min(n - 1)
    }
}

fn prop_value(prop: Prop, s: &EditorSettings) -> usize {
    match prop {
        Prop::VuSource => usize::from(s.vu_source),
        Prop::VuCal => usize::from(s.vu_high_cal),
    }
}

/// A lamp's brightness, in the 32 steps it is drawn in.
fn lamp_level(src: LampSrc, m: &Meters) -> f32 {
    let level = match src {
        LampSrc::Overload => m.overload,
        LampSrc::KeyInput => f32::from(u8::from(m.key_input)),
        LampSrc::DeEss => m.deess,
    };
    (level.clamp(0.0, 1.0) * 32.0).floor() / 32.0
}

/// The needle's position, in the 800 steps it is drawn in: a smaller move repaints nothing.
fn needle_step(m: &Meters) -> usize {
    (m.needle.clamp(0.0, 1.0) * 800.0) as usize
}

const BAR_MARKS: [f32; 11] = [0.0, -2.0, -4.0, -6.0, -8.0, -10.0, -12.0, -18.0, -24.0, -32.0, -42.0];
const GATE_MARKS: [f32; 5] = [30.0, 20.0, 15.0, 10.0, 5.0];

fn readouts(main: bool, m: &Meters) -> Vec<String> {
    let level = |v: f32| format!("{:.1}", v.max(-100.0));
    let mut lines = vec![
        format!("INPUT   {} dBFS", level(m.shown.input_db)),
        format!("OUTPUT   {} dBFS", level(m.shown.output_db)),
        format!("COMP GR   {} dB", level(m.shown.comp_gr)),
        format!("GATE GR   {} dB", level(m.shown.gate_gr)),
    ];
    if !main {
        lines.push(format!("DE-ESS   {}", if m.deess > 0.05 { "ACTIVE" } else { "IDLE" }));
        lines.push(format!("EXT KEY   {}", if m.key_input { "CONNECTED" } else { "NONE" }));
    }
    lines
}

fn vu_text(m: &Meters) -> String {
    if m.shown.vu < -30.0 {
        "< -30 VU".to_string()
    } else {
        format!("{:.1} VU", m.shown.vu)
    }
}

// ---- change detection ----

struct Sig(std::collections::hash_map::DefaultHasher);

impl Sig {
    fn f(&mut self, v: f32) -> &mut Self {
        v.to_bits().hash(&mut self.0);
        self
    }
    fn u(&mut self, v: usize) -> &mut Self {
        v.hash(&mut self.0);
        self
    }
    fn control(&mut self, c: Option<&Control>, keys: bool) -> &mut Self {
        if let Some(c) = c {
            (c.value().to_bits(), c.hover(), c.pressed(), c.focused() && keys).hash(&mut self.0);
        }
        self
    }
}

/// A number that changes whenever what widget `i` draws does.
pub fn signature(w: &Widget, i: usize, l: &Looks) -> u64 {
    let mut s = Sig(std::collections::hash_map::DefaultHasher::new());
    let v = l.values;
    let control = l.ctl[i].and_then(|id| l.controls.get(id));
    let hot = l.hot.filter(|h| h.0 == i).map_or(usize::MAX - 1, |h| h.1);
    let down = l.down.filter(|h| h.0 == i).map_or(usize::MAX - 1, |h| h.1);
    s.control(control, l.keys_active).u(hot).u(down);
    match &w.kind {
        Kind::Knob { param, .. } => {
            s.f(v[*param]).f(v[ix::SOURCE]).f(v[ix::LMFX3]).f(v[ix::HMFX3]).f(v[ix::LFHIGH]).f(v[ix::HFHIGH]);
        }
        Kind::Key { param, .. } | Kind::Route { param } | Kind::Toggle { param, .. } | Kind::Hero { param, .. } => {
            s.f(v[*param]);
        }
        Kind::Step { param, .. } | Kind::Seg { param, .. } => {
            s.f(v[*param]).f(v[ix::HQ]);
        }
        Kind::PropStep { prop, .. } | Kind::PropSeg { prop, .. } => {
            s.u(prop_value(*prop, &l.settings));
        }
        Kind::SkinKey => {
            s.u(usize::from(l.settings.skin));
        }
        Kind::Lamp { src, .. } => {
            s.u((lamp_level(*src, l.meters) * 32.0) as usize);
        }
        Kind::Vu => {
            s.u(needle_step(l.meters));
        }
        Kind::Bar { output } => {
            let db = if *output { l.meters.output_db } else { l.meters.input_db };
            s.u(BAR_MARKS.iter().filter(|m| db >= **m).count());
        }
        Kind::GateLeds => {
            s.u(GATE_MARKS.iter().filter(|m| l.meters.gate_gr >= **m).count());
        }
        Kind::Status { .. } => {
            s.u(l.hq as usize);
        }
        Kind::Slider { param, dim_unless_filters, .. } => {
            s.f(v[*param]).u(usize::from(*dim_unless_filters && !filters_active(v)));
        }
        Kind::Button { action, .. } => {
            s.u(usize::from(*action == Action::Page(l.page)));
        }
        Kind::Label { .. } => {}
        Kind::Readouts { main } => {
            readouts(*main, l.meters).hash(&mut s.0);
            s.u(usize::from(l.meters.overload > 0.0));
        }
        Kind::VuText => vu_text(l.meters).hash(&mut s.0),
        Kind::HqLamp => {
            s.f(v[ix::HQ]);
        }
        Kind::EqGraph => {
            // The spectrum to half a decibel, the response, and where the handles are.
            for db in &l.eq.spectrum.db {
                s.u((db.max(-100.0) * 2.0) as i32 as usize);
            }
            if let Some((curve, filter_curve)) = l.eq.curve() {
                for db in curve.iter().chain(filter_curve) {
                    s.f(*db);
                }
            }
            for param in [ix::ALLEQ, ix::BYPASS, ix::FILTROUTE, ix::SHELFROUTE, ix::PARAMROUTE, ix::LFFREQ, ix::HFFREQ, ix::LFHIGH, ix::HFHIGH, ix::LMFFREQ, ix::HMFFREQ, ix::LMFX3, ix::HMFX3, ix::HPF, ix::LPF] {
                s.f(v[param]);
            }
            s.f(l.sample_rate);
            // The handles move with their gains even while the equaliser is out and the curve flat.
            let plot = graphs::eq_plot(w.rect);
            for handle in 0..6 {
                let (hx, hy) = l.eq.handle(&plot, handle);
                s.f(hx).f(hy);
            }
        }
        Kind::CompGraph => {
            s.f(v[ix::COMPTYPE]).f(v[ix::THRESHOLD]).f(v[ix::RATIO]).f(v[ix::MAKEUP]);
        }
    }
    s.0.finish()
}

// ---- the static layer ----

/// A card of the Digital skin with its title.
fn card(s: &mut Scene, (x, y, w, h): R, title: &str) {
    s.fill_round(Rect::new(x, y, w, h), 5.0, c(D_CARD), c(D_CARD), true);
    s.line(Point::new(x + 12.0, y + 29.5), Point::new(x + w - 12.0, y + 29.5), alpha(D_LINE, 0.7), 1.0);
    text_l(s, x + 12.0, y + 15.0, title, c(D_INK), TEXT);
}

pub fn layer(s: &mut Scene, page: Page, vu: &VuParts) {
    let full = Rect::new(0.0, 0.0, page.size().0, page.size().1);
    if page != Page::Analogue {
        s.fill_rect(full, c(D_BACK), c(D_BACK), true);
        text_l(s, 14.0, 22.5, "Blue Channel", c(D_INK), 20.0);
        text_l(s, 138.0, 24.0, "DIGITAL", c(D_ACCENT), SMALL);
        if page == Page::Overview {
            for (r, title) in OVERVIEW_CARDS {
                card(s, r, title);
            }
            eq_graph_grid(s, EQ_GRAPH);
            comp_graph_grid(s, COMP_GRAPH);
        } else {
            for (r, title) in CARDS {
                card(s, r, title);
            }
        }
        return;
    }
    let t = &ANALOGUE;
    hardware::draw_brushed_panel(s, full, c(t.panel_top), c(t.panel_bottom), 7);
    for ((x, y, w, h), title) in SECTIONS {
        let r = Rect::new(x, y, w, h);
        s.fill_round(r.translated(0.0, 2.0), 7.0, c(0x55000000), c(0x55000000), true);
        s.fill_round(r, 7.0, c(t.section), c(t.section), true);
        s.stroke_round(r, 7.0, c(t.section_edge), 1.0);
        heavy_r(s, x + w - 8.0, y + h - 10.0, title, c(t.ink_dim), A_TITLE);
    }
    // The brand block, bottom left.
    let ink = c(t.ink_on_panel);
    text_l(s, 22.0, 253.0, "Blue Channel", ink, 27.0);
    heavy(s, 23.0, 275.0, "analogue-modelled channel strip", alpha(t.ink_on_panel, 0.92), 11.0);
    // The OpenGrid mark: a 3 x 3 grid of squares.
    for gy in 0..3 {
        for gx in 0..3 {
            let a = if (gx + gy) % 2 == 1 { 0.55 } else { 1.0 };
            s.fill_round(Rect::new(24.0 + 8.0 * gx as f32 + 1.0, 392.0 + 8.0 * gy as f32 + 1.0, 6.0, 6.0), 1.0, alpha(t.ink_on_panel, a), alpha(t.ink_on_panel, a), true);
        }
    }
    text_l(s, 54.0, 404.0, "OpenGrid", ink, 15.0);
    text_r(s, 240.0, 405.0, concat!("v", env!("CARGO_PKG_VERSION")), alpha(t.ink_on_panel, 0.9), A_LAMP);
    // The meter captions, and the output bar's scale, which never changes.
    heavy_c(s, 1420.0, 20.0, "Out", ink, A_LAMP);
    heavy_c(s, 1454.0, 20.0, "In", ink, A_LAMP);
    heavy_c(s, 1432.0, 190.0, "dBFS", alpha(t.ink_on_panel, 0.92), A_LAMP);
    let step = OUT_BAR.3 / BAR_MARKS.len() as f32;
    for (n, db) in BAR_MARKS.iter().enumerate() {
        heavy_r(s, bar_column(OUT_BAR).0 - 3.0, OUT_BAR.1 + step * (n as f32 + 0.5), &format!("{}", *db as i32), ink, A_LAMP);
    }
    s.cmds.extend(vu.face());
}

// ---- the widgets ----

fn ink_c(on_section: bool) -> Color {
    c(if on_section { ANALOGUE.ink_on_section } else { ANALOGUE.ink_on_panel })
}

/// Type on the analogue panel, in panel units. The release printed it at 7.5 to 10; these
/// are the largest each kind of legend has room for where it stands.
const A_KEY: f32 = 10.5;
const A_LAMP: f32 = 10.5;
const A_CAPTION: f32 = 11.5;
const A_LEGEND: f32 = 10.0;
const A_TITLE: f32 = 11.0;

/// The largest of `sizes` at which `text` is no wider than `room`; the last if none is.
fn fitting(text: &str, room: f32, sizes: &[f32]) -> f32 {
    sizes.iter().copied().find(|size| lettering::width(text, *size, true) <= room).unwrap_or(sizes[sizes.len() - 1])
}

/// A square key with an LED cap, its legend below.
#[allow(clippy::too_many_arguments)]
fn led_key(s: &mut Scene, r: R, label: &str, led: u32, lit: bool, over: bool, down: bool, on_section: bool) {
    let key = Rect::new(r.0 + r.2 * 0.5 - 10.0, r.1 + 2.0, 20.0, 12.0);
    hardware::draw_led_key(s, key, c(led), lit, down);
    if over {
        s.fill_round(key.expanded(2.5), 3.0, c(0x22ffffff), c(0x22ffffff), true);
    }
    // A legend may run a little past its key, never into the next one's.
    let size = fitting(label, r.2 + 6.0, &[A_KEY, 10.0, 9.5, 9.0]);
    heavy_c(s, r.0 + r.2 * 0.5, r.1 + 24.5, label, ink_c(on_section), size);
}

/// An indicator lamp with its legend to the right. `look` is the lamp's radius and the
/// legend's size.
fn lamp(s: &mut Scene, r: R, label: &str, colour: u32, level: f32, text_colour: Color, look: (f32, f32)) {
    let (radius, size) = look;
    let at = Point::new(r.0 + radius * 2.0, r.1 + r.3 * 0.5);
    let tint = c(0xff000000).mix(c(colour), 0.35 + 0.65 * level);
    meters::draw_lamp(s, at, radius, tint, level > 0.05);
    if radius < 4.0 {
        heavy(s, r.0 + radius * 5.0, r.1 + r.3 * 0.5, label, text_colour, size);
    } else {
        text_l(s, r.0 + radius * 5.0, r.1 + r.3 * 0.5, label, text_colour, size);
    }
}

fn ring(s: &mut Scene, r: Rect, round: bool) {
    let colour = c(0xff5cc8ff);
    if round {
        s.stroke_ellipse(r.expanded(3.0), colour, 1.6);
    } else {
        s.stroke_round(r.expanded(3.0), 5.0, colour, 1.6);
    }
}

/// A flat button of the Digital skin.
fn flat_key(s: &mut Scene, r: R, label: &str, on: bool, over: bool, size: f32) {
    let b = Rect::new(r.0 + 1.0, r.1 + 1.0, r.2 - 2.0, r.3 - 2.0);
    let fill = c(if on { 0xff17445a } else { 0xff101e2e });
    let fill = if over { fill.brighter(0.15) } else { fill };
    s.fill_round(b, 4.0, fill, fill, true);
    s.stroke_round(b, 4.0, c(if on { D_ACCENT } else { D_LINE }), 1.0);
    text_c(s, r.0 + r.2 * 0.5, r.1 + r.3 * 0.5, label, c(if on { D_ACCENT } else { D_INK }), size);
}

fn segments(s: &mut Scene, r: R, names: &[&str], chosen: usize, hot: Option<usize>) {
    let w = r.2 / names.len() as f32;
    for (i, name) in names.iter().enumerate() {
        flat_key(s, (r.0 + w * i as f32, r.1, w, r.3), name, i == chosen, hot == Some(i), SMALL);
    }
}

/// The lamps of the analogue panel.
const A_LAMP_R: f32 = 3.3;

pub fn widget(s: &mut Scene, w: &Widget, i: usize, l: &Looks) {
    let v = l.values;
    let (x, y, width, height) = w.rect;
    let control = l.ctl[i].and_then(|id| l.controls.get(id));
    let over = control.is_some_and(Control::hover);
    let pressed = control.is_some_and(Control::pressed);
    let focused = l.keys_active && control.is_some_and(Control::focused);
    let hot = l.hot.filter(|h| h.0 == i).map(|h| h.1);
    let down = l.down.filter(|h| h.0 == i).map(|h| h.1);
    match &w.kind {
        Kind::Knob { param, item, lo, hi, on_section, enabled } => {
            let t = &ANALOGUE;
            let (cx, cy, radius) = knob_circle(w.rect);
            heavy_c(s, cx, y + KNOB_CAPTION * 0.5, &item.label, ink_c(*on_section), A_CAPTION);
            let backdrop = c(if *on_section { t.section } else { t.panel_top });
            draw::set_knob_floor(backdrop);
            let accent = if *enabled { Color::from_hex(&item.colour).unwrap_or(c(0xff697170)) } else { c(0xff697170) };
            let angle = (PARAMS[*param].to_normalized(v[*param]) * 1.6 - 0.8) * std::f32::consts::PI;
            let dark = hardware::use_dark_marks(&item.marks, backdrop);
            draw::knob_named_mode(s, &item.style, Point::new(cx, cy), radius, angle, accent, dark, KNOBS);
            if focused {
                ring(s, Scene::ellipse_at(Point::new(cx, cy), radius), true);
            }
            if (over || pressed || focused) && *enabled {
                // The value takes the legends' place: `overlay` draws it over everything.
                return;
            }
            let ly = y + height - KNOB_LEGEND * 0.5;
            let dim = if *on_section { c(t.ink_dim) } else { alpha(t.ink_on_panel, 0.9) };
            let (lo, hi) = legends(*param, lo, hi, v);
            // As large as the two fit side by side with a gap, at the ends of the line.
            let size = [A_LEGEND, 9.5, 9.0, 8.5].into_iter().find(|size| lettering::width(lo, *size, true) + lettering::width(hi, *size, true) + 7.0 <= width).unwrap_or(8.0);
            // Two units in from the ends where there is room, so neighbours' legends stay apart.
            let inset = ((width - lettering::width(lo, size, true) - lettering::width(hi, size, true) - 7.0) * 0.5).clamp(0.0, 2.0);
            heavy(s, x + inset, ly, lo, dim, size);
            heavy_r(s, x + width - inset, ly, hi, dim, size);
            if let Some(extra) = extra_legend(*param, v) {
                heavy_c(s, cx, ly, extra, ink_c(*on_section), A_LEGEND);
            }
        }
        Kind::Key { param, label, led, on_section } => {
            led_key(s, w.rect, label, *led, v[*param] > 0.5, over, pressed, *on_section);
            if focused {
                ring(s, rect(w.rect), false);
            }
        }
        Kind::Route { param } => {
            let third = width / 3.0;
            let value = v[*param].round() as usize;
            for (k, (label, led)) in ROUTE_KEYS.iter().enumerate() {
                led_key(s, (x + third * k as f32, y, third, height), label, *led, value == k + 1, hot == Some(k), down == Some(k), true);
            }
        }
        Kind::Step { param, label, names, led, on_section, key_w, hq } => {
            led_key(s, step_key(w.rect, *key_w), label, *led, false, hot == Some(0), down == Some(0), *on_section);
            let rows = step_rows(w.rect, *key_w, names.len());
            let h = rows.3 / names.len() as f32;
            let chosen = selected(*param, *hq, v, names.len());
            for (n, name) in names.iter().enumerate() {
                lamp(s, (rows.0, rows.1 + h * n as f32, rows.2, h), name, *led, f32::from(u8::from(n == chosen)), ink_c(*on_section), (A_LAMP_R, A_LAMP));
            }
            if focused {
                ring(s, rect(rows), false);
            }
        }
        Kind::PropStep { prop, label, names } => {
            led_key(s, (x, y, PROP_KEY, height), label, LED_RED, false, hot == Some(0), down == Some(0), false);
            let rows = prop_rows(w.rect, names.len());
            let h = rows.3 / names.len() as f32;
            for (n, name) in names.iter().enumerate() {
                lamp(s, (rows.0, rows.1 + h * n as f32, rows.2, h), name, LED_RED, f32::from(u8::from(n == prop_value(*prop, &l.settings))), ink_c(false), (A_LAMP_R, if names.len() > 2 { 10.0 } else { A_LAMP }));
            }
        }
        Kind::SkinKey => led_key(s, w.rect, "Digital skin", LED_GREEN, l.settings.skin == 1, hot.is_some(), down.is_some(), false),
        Kind::Lamp { label, colour, on_section, src } => lamp(s, w.rect, label, *colour, lamp_level(*src, l.meters), ink_c(*on_section), (A_LAMP_R, A_LAMP)),
        Kind::Vu => l.vu.draw(s, needle_step(l.meters) as f32 / 800.0),
        Kind::Bar { output } => {
            let level = if *output { l.meters.output_db } else { l.meters.input_db };
            let step = height / BAR_MARKS.len() as f32;
            let column = bar_column(w.rect);
            for (n, db) in BAR_MARKS.iter().enumerate() {
                let colour = c(if *db >= -2.0 { LED_RED } else if *db >= -10.0 { LED_AMBER } else { LED_GREEN });
                let cell = Rect::new(column.0, y + step * n as f32, column.2, step).reduced(1.0, 1.2);
                let on = level >= *db;
                let fill = if on { colour } else { c(0xff000000).mix(colour, 0.22) };
                if on {
                    s.fill_round(cell.expanded(1.5), 2.0, colour.with_alpha(0.25), colour.with_alpha(0.25), true);
                }
                s.fill_round(cell, 1.2, fill, fill, true);
            }
        }
        Kind::GateLeds => {
            let cell = width / GATE_MARKS.len() as f32;
            for (n, mark) in GATE_MARKS.iter().enumerate() {
                let cx = x + cell * (n as f32 + 0.5);
                meters::draw_lamp(s, Point::new(cx, y + 7.0), 3.0, c(LED_GREEN), l.meters.gate_gr >= *mark);
                heavy_c(s, cx, y + 17.5, &format!("-{}", *mark as i32), c(ANALOGUE.ink_on_section), 10.0);
            }
        }
        Kind::Status { size, colour, centre } => {
            if *centre {
                heavy_c(s, x + width * 0.5, y + height * 0.5, l.hq.text(), c(*colour), *size);
            } else {
                text_l(s, x, y + height * 0.5, l.hq.text(), c(*colour), *size);
            }
        }

        Kind::Slider { param, name, dim_unless_filters, enabled } => {
            let dim = *dim_unless_filters && !filters_active(v);
            let fade = |colour: u32| if dim { c(D_CARD).mix(c(colour), 0.55) } else { c(colour) };
            let (x0, x1, cy) = slider_track(w.rect);
            text_l(s, x, cy, name, fade(D_INK), TEXT);
            s.fill_round(Rect::new(x0, cy - 2.5, x1 - x0, 5.0), 2.5, fade(0xff2b4059), fade(0xff2b4059), true);
            let at = x0 + (x1 - x0) * PARAMS[*param].to_normalized(v[*param]);
            let accent = fade(if *enabled { D_ACCENT } else { 0xff6f8299 });
            s.fill_round(Rect::new(x0, cy - 2.5, (at - x0).max(0.0), 5.0), 2.5, accent, accent, true);
            let thumb = if over || pressed { 7.0 } else { 6.0 };
            s.fill_ellipse_solid(Scene::ellipse_at(Point::new(at, cy), thumb), accent);
            if focused {
                ring(s, Scene::ellipse_at(Point::new(at, cy), 7.0), true);
            }
            text_r(s, x + width, cy, &text::value(*param, v[*param]), fade(if *enabled { D_ACCENT } else { D_DIM }), TEXT);
        }
        Kind::Toggle { param, name } => {
            flat_key(s, w.rect, name, v[*param] > 0.5, over || pressed, SMALL);
            if focused {
                ring(s, rect(w.rect), false);
            }
        }
        Kind::Seg { param, name, names, hq } => {
            text_l(s, x, y + height * 0.5, name, c(D_INK), TEXT);
            let hover = if *hq { hot } else { None };
            segments(s, seg_rect(w.rect), names, selected(*param, *hq, v, names.len()), hover);
            if focused {
                ring(s, rect(seg_rect(w.rect)), false);
            }
        }
        Kind::PropSeg { prop, names } => segments(s, w.rect, names, prop_value(*prop, &l.settings), hot),
        Kind::Button { text, action } => flat_key(s, w.rect, text, *action == Action::Page(l.page), hot.is_some(), SMALL),
        Kind::Hero { param, enabled } => {
            let (cx, cy, radius) = hero_circle(w.rect);
            draw::set_knob_floor(c(D_CARD));
            let accent = c(if *enabled { D_ACCENT } else { 0xff53657b });
            let angle = (PARAMS[*param].to_normalized(v[*param]) * 1.6 - 0.8) * std::f32::consts::PI;
            draw::knob_named_mode(s, "Neo", Point::new(cx, cy), radius, angle, accent, false, KNOBS);
            if focused {
                ring(s, Scene::ellipse_at(Point::new(cx, cy), radius), true);
            }
            let name = if *param == ix::OUTPUT { "Output" } else if *param == ix::MICGAIN { "Mic gain" } else { "Line gain" };
            let value = format!("{name}   {}", text::value(*param, v[*param]));
            text_c(s, cx, y + height - HERO_VALUE * 0.5 + 2.0, &value, c(if *enabled { D_ACCENT } else { D_DIM }), 14.0);
        }
        Kind::Label { text } => text_l(s, x, y + height * 0.5, text, c(D_INK), TEXT),
        Kind::Readouts { main } => {
            let (pitch, size) = if *main { (37.0, 15.0) } else { (25.0, TEXT) };
            for (n, line) in readouts(*main, l.meters).iter().enumerate() {
                let colour = if n == 0 && l.meters.overload > 0.0 { 0xffff7a6b } else { D_ACCENT };
                text_l(s, x, y + pitch * (n as f32 + 0.5), line, c(colour), size);
            }
        }
        Kind::VuText => text_l(s, x, y + height * 0.5, &vu_text(l.meters), c(D_INK), TEXT),
        Kind::HqLamp => lamp(s, w.rect, "HQ", LED_AMBER, f32::from(u8::from(v[ix::HQ] > 0.5)), c(D_INK), (4.0, SMALL)),
        Kind::EqGraph => eq_graph(s, w.rect, l),
        Kind::CompGraph => comp_graph(s, w.rect, l),
    }
}

/// What a widget shows over everything else on the page: the value of a knob that is held,
/// pointed at or focused, on a dark plate over the legend line. It is larger than the legends
/// and may cover the ends of the neighbours' while it shows.
pub fn overlay(s: &mut Scene, w: &Widget, i: usize, l: &Looks) {
    let Kind::Knob { param, enabled: true, .. } = &w.kind else { return };
    let Some(control) = l.ctl[i].and_then(|id| l.controls.get(id)) else { return };
    if !(control.hover() || control.pressed() || (l.keys_active && control.focused())) {
        return;
    }
    let (cx, _, _) = knob_circle(w.rect);
    let ly = w.rect.1 + w.rect.3 - KNOB_LEGEND * 0.5;
    let value = text::value(*param, l.values[*param]);
    let size = fitting(&value, PLATE_W - 12.0, &[PLATE_TEXT, 13.0, 12.0, 11.0]);
    let plate_w = (lettering::width(&value, size, true) + 14.0).min(PLATE_W);
    let plate = Rect::new(cx - plate_w * 0.5, ly - PLATE_H * 0.5, plate_w, PLATE_H);
    s.fill_round(plate, 4.5, c(0xff171a21), c(0xff171a21), true);
    s.stroke_round(plate, 4.5, c(0xffe8ecf2), 1.0);
    text_c(s, cx, ly, &value, c(0xffffffff), size);
}

/// The equaliser graph's background, grid and scales: static, so part of the layer.
fn eq_graph_grid(s: &mut Scene, r: R) {
    let a = graphs::eq_plot(r);
    s.fill_round(rect(r), 4.0, c(0xff050a12), c(0xff050a12), true);
    for f in [50.0f32, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0] {
        let x = graphs::eq_x(&a, f).round();
        s.line(Point::new(x, a.y), Point::new(x, a.bottom()), alpha(D_LINE, 0.3), 1.0);
        let label = if f >= 1000.0 { format!("{}k", f / 1000.0) } else { format!("{f}") };
        text_c(s, x, a.bottom() + 14.0, &label, c(D_DIM), SMALL);
    }
    for d in [-18.0f32, -12.0, -6.0, 0.0, 6.0, 12.0, 18.0] {
        let y = graphs::eq_y(&a, d).round();
        s.line(Point::new(a.x, y), Point::new(a.right(), y), alpha(D_LINE, if d == 0.0 { 0.7 } else { 0.25 }), 1.0);
        text_r(s, a.x - 8.0, y, &format!("{d}"), c(D_DIM), SMALL);
    }
    text_r(s, a.right(), r.1 + 14.0, "OUTPUT L / FFT", c(D_DIM), SMALL);
    text_l(s, a.x, r.1 + 14.0, "DRAG A HANDLE FOR FREQUENCY AND GAIN / SCROLL A MID BAND FOR Q", c(D_DIM), SMALL);
}

fn eq_graph(s: &mut Scene, r: R, l: &Looks) {
    let v = l.values;
    let a = graphs::eq_plot(r);
    // The output spectrum: -100 dB at the bottom, 0 dB at the top.
    let half = graphs::Spectrum::SIZE / 2;
    let rate = l.sample_rate.max(1.0);
    let level = |x: f32| {
        let f = graphs::eq_hz(&a, x);
        let bin = ((f / rate * graphs::Spectrum::SIZE as f32) as usize).clamp(1, half - 1);
        let db = if f <= rate * 0.5 { l.eq.spectrum.db[bin] } else { -100.0 };
        ((db + 100.0) / 100.0).clamp(0.0, 1.0)
    };
    // Under it a band of light that fades out, as columns with line segments on top. Every pixel filled costs the same whatever fills it (the renderer works out a
    // signed distance and the clips for each), so the band is kept shallow: filling down to
    // the floor of the plot cost 8 ms a frame at this size. Polygon fills and wide
    // polylines cost a hundred times as much again.
    const BAND: f32 = 56.0;
    let mut tops = Vec::with_capacity((a.w / 2.0) as usize + 2);
    let mut px = 0.0;
    while px <= a.w {
        tops.push(Point::new(a.x + px, a.bottom() - level(a.x + px) * a.h));
        px += 2.0;
    }
    for column in tops.chunks(3).filter(|column| column.len() > 1) {
        // Six units wide (three points): the cost is as much per rectangle as per pixel.
        let top = column.iter().map(|p| p.y).fold(f32::MAX, f32::min);
        let depth = BAND.min(a.bottom() - top);
        if depth > 0.5 {
            let width = (column[column.len() - 1].x - column[0].x + 2.0).min(a.right() - column[0].x);
            s.fill_rect(Rect::new(column[0].x, top, width, depth), alpha(D_ACCENT, 0.36), alpha(D_ACCENT, 0.36 * (1.0 - depth / BAND)), true);
        }
    }
    for pair in tops.windows(2) {
        s.line(pair[0], pair[1], c(0x6639d0ff), 1.0);
    }
    s.clip_round(Rect::new(a.x, a.y, a.w, a.h), 0.0);
    if let Some((curve, filter_curve)) = l.eq.curve() {
        let path = |curve: &[f32]| -> Vec<Point> { curve.iter().enumerate().map(|(i, db)| Point::new(a.x + i as f32 / (curve.len() - 1) as f32 * a.w, graphs::eq_y(&a, *db))).collect() };
        let response = path(curve);
        for (colour, width) in [(alpha(D_ACCENT, 0.13), 7.0), (c(D_INK), 1.7)] {
            for pair in response.windows(2) {
                s.line(pair[0], pair[1], colour, width);
            }
        }
        if v[ix::FILTROUTE].round() >= 2.0 {
            // The filters feed a key: their own response, dashed.
            for pair in path(filter_curve).chunks(2) {
                if let [p, q] = pair {
                    s.line(*p, *q, c(D_KEY), 1.3);
                }
            }
        }
    }
    s.pop_clip();
    const NAMES: [&str; 6] = ["LF", "LMF", "HMF", "HF", "HP", "LP"];
    const COLOURS: [u32; 4] = [0xffff6a5a, 0xff5ae07a, 0xff5ab4ff, 0xffffc04a];
    let eq_on = v[ix::ALLEQ] > 0.5 && v[ix::BYPASS] < 0.5;
    for i in 0..6 {
        let (hx, hy) = l.eq.handle(&a, i);
        let route = v[if i >= 4 { ix::FILTROUTE } else if i == 0 || i == 3 { ix::SHELFROUTE } else { ix::PARAMROUTE }].round() as usize;
        let key = route >= 2;
        let on = key || (eq_on && route == 1);
        let hue = if key { D_KEY } else if i >= 4 { D_ACCENT } else { COLOURS[i] };
        // A band that is out of the path is dimmed, its name less so: it still has to read.
        let colour = alpha(hue, if on { 1.0 } else { 0.35 });
        s.fill_ellipse_solid(Scene::ellipse_at(Point::new(hx, hy), 13.0), colour.with_alpha(colour.alpha_f() * 0.18));
        s.stroke_ellipse(Scene::ellipse_at(Point::new(hx, hy), 6.5), colour, 2.2);
        let title = format!("{}{}", NAMES[i], if !key { "" } else if route == 2 { " / COMP" } else { " / GATE" });
        let lx = (hx - 50.0).clamp(r.0 + 2.0, r.0 + r.2 - 102.0) + 50.0;
        text_c(s, lx, hy + if i % 2 == 1 { 22.0 } else { -22.0 }, &title, alpha(hue, if on { 1.0 } else { 0.7 }), SMALL);
    }
}

fn comp_graph_grid(s: &mut Scene, r: R) {
    let a = graphs::comp_plot(r);
    s.fill_round(rect(r), 4.0, c(0xff050a12), c(0xff050a12), true);
    for d in [-40.0f32, -20.0, 0.0, 20.0] {
        let (x, y) = (graphs::comp_x(&a, d).round(), graphs::comp_y(&a, d).round());
        s.line(Point::new(x, a.y), Point::new(x, a.bottom()), alpha(D_LINE, 0.3), 1.0);
        s.line(Point::new(a.x, y), Point::new(a.right(), y), alpha(D_LINE, 0.3), 1.0);
        text_c(s, x, a.bottom() + 14.0, &format!("{d}"), c(D_DIM), SMALL);
        text_r(s, a.x - 6.0, y, &format!("{d}"), c(D_DIM), SMALL);
    }
    text_l(s, a.x, r.1 + 14.0, "DETECTOR TARGET / dBu", c(D_DIM), SMALL);
}

fn comp_graph(s: &mut Scene, r: R, l: &Looks) {
    let a = graphs::comp_plot(r);
    let curve = CompCurve::new(l.values);
    s.clip_round(Rect::new(a.x, a.y, a.w, a.h), 0.0);
    s.line(Point::new(graphs::comp_x(&a, -40.0), graphs::comp_y(&a, -40.0)), Point::new(graphs::comp_x(&a, 20.0), graphs::comp_y(&a, 20.0)), c(D_LINE), 1.0);
    let path: Vec<Point> = (-40..=20).map(|d| Point::new(graphs::comp_x(&a, d as f32), graphs::comp_y(&a, curve.output(d as f32)))).collect();
    for pair in path.windows(2) {
        s.line(pair[0], pair[1], c(D_ACCENT), 2.0);
    }
    s.pop_clip();
    for (i, (name, colour)) in [("THRESH", D_ACCENT), ("RATIO", 0xffffc04a), ("MAKEUP", 0xff5ae07a)].into_iter().enumerate() {
        let (hx, hy) = curve.handle(&a, i);
        s.fill_ellipse_solid(Scene::ellipse_at(Point::new(hx, hy), 6.0), c(colour));
        text_c(s, hx, hy + if i == 1 { -16.0 } else { 18.0 }, name, c(colour), SMALL);
    }
}
