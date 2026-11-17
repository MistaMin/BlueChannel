//! The editor: the analogue panel and the Digital skin of the 1.0 release,
//! drawn with GoodLookin DRI. Every control is bound to a host parameter, so
//! automation and session restore act on the parameter and the editor only
//! reflects it.
//!
//! Each page is laid out at a fixed size in its own units and scaled as a whole
//! into the window: the analogue panel is 1560 x 450 units (the hardware's front
//! panel), the Digital pages 1250 x 900. The window is the page's size times one
//! saved scale (half to twice, same shape); when the skin changes the editor asks
//! the host for the other page's size, and until the host gives it (or if it
//! never does) the page is fitted into the window it has, centred.

mod graphs;
mod lettering;
mod paint;
mod panel;
mod text;

use std::sync::atomic::Ordering::Relaxed;
use std::sync::Arc;

use openplugin::core::editor::{AccessAction, AccessNode, AccessRange, ParamMenu, ParamMenuAction, SizeRange, TextEdit, Tooltip};
use openplugin::core::{Editor, EditorHost, EditorState, Frame};
use openplugin::dri::goodlookindri::meters::vu_position_for_db;
use openplugin::dri::goodlookindri::scene::Scene;
use openplugin::dri::goodlookindri::{draw, hardware};
use openplugin::dri::{Area, Control, ControlSet, DamageRect, DamageTracker, DoubleClick, Key, Modifiers, SceneCanvas};

use graphs::{CompCurve, EqGraph};
use paint::{Looks, Meters, VuParts};
pub use panel::Page;
use panel::{Action, Kind, Prop, Widget};

use crate::params::index as ix;
use crate::{EditorSettings, HqStatus, Shared, COUNT, PARAMS};

/// 0 dBFS as a peak voltage and 0 dBu in volts (Source/Model/Levels.h).
const FULL_SCALE_PEAK: f32 = 13.790_73; // +22 dBu
const DBU_VOLTS: f32 = 0.774_596_7;

/// A graph handle being dragged, and the parameters it has opened a gesture on.
struct GraphDrag {
    widget: usize,
    handle: usize,
    open: Vec<usize>,
}

pub struct BlueEditor {
    host: EditorHost,
    shared: Option<Arc<Shared>>,
    /// The settings while no engine is bound.
    local: EditorSettings,
    values: [f32; COUNT],
    /// The Digital skin shows its "all controls" page.
    details: bool,
    page: Page,
    /// The input the widgets were built for.
    source: usize,
    size: (u32, u32),
    widgets: Vec<Widget>,
    /// The control of each widget that has one.
    ctl: Vec<Option<usize>>,
    controls: ControlSet,
    canvas: SceneCanvas,
    /// What the next frame has to repaint, in panel units.
    damage: DamageTracker,
    signatures: Vec<u64>,
    meters: Meters,
    hq: HqStatus,
    eq: EqGraph,
    vu: VuParts,
    hot: Option<(usize, usize)>,
    down: Option<(usize, usize)>,
    drag: Option<GraphDrag>,
    modifiers: Modifiers,
    keys_active: bool,
}

pub fn make_editor(host: EditorHost) -> Option<Box<dyn Editor>> {
    Some(Box::new(BlueEditor::new(host)))
}

/// The window of a page at `scale` points per panel unit.
fn size_for(page: Page, scale: f32) -> (u32, u32) {
    let (width, height) = page.size();
    ((width * scale).round() as u32, (height * scale).round() as u32)
}

/// The sizes the window of a page can take: half to twice its panel, in its shape.
fn range(page: Page) -> SizeRange {
    let (width, height) = page.size();
    SizeRange::between(size_for(page, 0.5), size_for(page, 2.0)).with_aspect(width as u32, height as u32)
}

/// The canvas takes at most this many separate rectangles as a partial repaint, and no more
/// than this share of the frame (`openplugin-dri`'s `MAX_PIXEL_RECTS` and `FULL_SHARE`, with
/// room to spare for its own rounding).
const MAX_PIECES: usize = 6;
const MAX_SHARE: f64 = 0.5;

type PixelRect = (usize, usize, usize, usize);

/// Damage in panel units (the panel at `unit` pixels per unit, its corner at (`left`, `top`)
/// pixels) as at most `MAX_PIECES` pixel rectangles (x0, y0, x1, y1) that do
/// not touch: rounded outwards, joined where they touch, and the nearest joined until few
/// enough are left. `None` when repainting everything is the better choice.
fn pieces(damage: &[DamageRect], unit: f32, (left, top): (f32, f32), (width, height): (usize, usize)) -> Option<Vec<PixelRect>> {
    let join = |a: PixelRect, b: PixelRect| (a.0.min(b.0), a.1.min(b.1), a.2.max(b.2), a.3.max(b.3));
    let area = |r: PixelRect| (r.2 - r.0) * (r.3 - r.1);
    let mut out: Vec<PixelRect> = Vec::new();
    let add = |out: &mut Vec<PixelRect>, mut r: PixelRect| {
        // One pixel apart counts as touching: the canvas rounds once more.
        while let Some(i) = out.iter().position(|o| r.0 <= o.2 + 1 && o.0 <= r.2 + 1 && r.1 <= o.3 + 1 && o.1 <= r.3 + 1) {
            r = join(r, out.swap_remove(i));
        }
        out.push(r);
    };
    for d in damage {
        if !d.is_finite() {
            return None;
        }
        let x0 = (d.x * unit + left).floor().clamp(0.0, width as f32) as usize;
        let y0 = (d.y * unit + top).floor().clamp(0.0, height as f32) as usize;
        let x1 = ((d.x + d.w) * unit + left).ceil().clamp(0.0, width as f32) as usize;
        let y1 = ((d.y + d.h) * unit + top).ceil().clamp(0.0, height as f32) as usize;
        if x1 > x0 && y1 > y0 {
            add(&mut out, (x0, y0, x1, y1));
        }
    }
    while out.len() > MAX_PIECES {
        // Join the two whose common rectangle adds the least to repaint.
        let mut best = (usize::MAX, 0, 1);
        for i in 0..out.len() {
            for j in i + 1..out.len() {
                let extra = area(join(out[i], out[j])).saturating_sub(area(out[i]) + area(out[j]));
                if extra < best.0 {
                    best = (extra, i, j);
                }
            }
        }
        let (b, a) = (out.swap_remove(best.2), out.swap_remove(best.1));
        add(&mut out, join(a, b));
    }
    let total: usize = out.iter().map(|r| area(*r)).sum();
    (total as f64 <= MAX_SHARE * (width * height) as f64).then_some(out)
}

impl BlueEditor {
    pub fn new(host: EditorHost) -> Self {
        let local = EditorSettings::default();
        let mut editor = Self {
            host: host.clone(),
            shared: None,
            local,
            values: std::array::from_fn(|i| PARAMS[i].default),
            details: false,
            page: Page::Analogue,
            source: 0,
            size: size_for(Page::Analogue, local.ui_scale),
            widgets: Vec::new(),
            ctl: Vec::new(),
            controls: ControlSet::new(host),
            canvas: SceneCanvas::new(Page::Analogue.size().0, Page::Analogue.size().1),
            damage: DamageTracker::new(),
            signatures: Vec::new(),
            meters: Meters::default(),
            hq: HqStatus::Idle,
            eq: EqGraph::default(),
            vu: VuParts::default(),
            hot: None,
            down: None,
            drag: None,
            modifiers: Modifiers::NONE,
            keys_active: true,
        };
        editor.rebuild();
        editor
    }

    /// How the page shown sits in the window: points per panel unit, and where the panel's
    /// top left corner is, in points. The window normally has the page's shape and the corner
    /// is (0, 0); in any other window (a host that did not give the size asked for) the
    /// panel is as large as fits, whole, in the middle.
    fn fit(&self) -> (f32, f32, f32) {
        let (width, height) = self.page.size();
        let (w, h) = (self.size.0 as f32, self.size.1 as f32);
        let k = (w / width).min(h / height);
        (k, ((w - width * k) * 0.5).floor().max(0.0), ((h - height * k) * 0.5).floor().max(0.0))
    }

    /// Points per panel unit.
    fn k(&self) -> f32 {
        self.fit().0
    }

    /// A point of the window in panel units.
    fn to_panel(&self, x: f32, y: f32) -> (f32, f32) {
        let (k, left, top) = self.fit();
        ((x - left) / k, (y - top) / k)
    }

    /// A rectangle of the panel in points.
    fn to_window(&self, r: panel::R) -> DamageRect {
        let (k, left, top) = self.fit();
        DamageRect::new(r.0 * k + left, r.1 * k + top, r.2 * k, r.3 * k)
    }

    /// Ask the host for the window of the page shown, at the saved scale.
    fn ask_for_size(&self) {
        let wanted = size_for(self.page, self.settings().ui_scale);
        if wanted != self.size {
            self.host.request_resize(&range(self.page), wanted.0, wanted.1);
        }
    }

    pub fn settings(&self) -> EditorSettings {
        self.shared.as_ref().map_or(self.local, |s| s.settings())
    }

    fn set_settings(&mut self, settings: EditorSettings) {
        self.local = settings;
        if let Some(shared) = &self.shared {
            shared.set_settings(settings);
        }
    }

    pub fn page(&self) -> Page {
        self.page
    }

    /// Show a skin and page (0 analogue; 1 Digital, its overview or all its controls).
    pub fn show(&mut self, skin: u8, details: bool) {
        self.details = details;
        self.set_settings(EditorSettings { skin, ..self.settings() });
        let before = self.page.size();
        self.rebuild();
        // The other skin's panel has another shape.
        if self.page.size() != before {
            self.ask_for_size();
        }
    }

    pub fn values(&self) -> &[f32; COUNT] {
        &self.values
    }

    /// The widgets of the page shown, for tests and tooling: each one's
    /// rectangle in panel units and the parameters it controls.
    pub fn widget_params(&self) -> Vec<(panel::R, Vec<usize>)> {
        self.widgets.iter().map(|w| (w.rect, w.params())).collect()
    }

    fn wanted_page(&self) -> Page {
        match (self.settings().skin, self.details) {
            (0, _) => Page::Analogue,
            (_, false) => Page::Overview,
            (_, true) => Page::Controls,
        }
    }

    /// Build the page's widgets and their controls at the current size.
    fn rebuild(&mut self) {
        self.page = self.wanted_page();
        self.source = self.values[ix::SOURCE].round().clamp(0.0, 2.0) as usize;
        self.widgets = panel::widgets(self.page, self.source);
        let (k, left, top) = self.fit();
        let scaled = |r: panel::R| Area::rect(r.0 * k + left, r.1 * k + top, r.2 * k, r.3 * k);
        let circle = |c: (f32, f32, f32)| Area::circle(c.0 * k + left, c.1 * k + top, c.2 * k);
        let mut controls = ControlSet::new(self.host.clone()).with_double_click(DoubleClick::Reset);
        self.ctl = self
            .widgets
            .iter()
            .map(|w| {
                let control = match &w.kind {
                    Kind::Knob { param, enabled: true, .. } => Control::knob(circle(paint::knob_circle(w.rect)), *param, &PARAMS[*param]).drag_range(220.0 * k),
                    Kind::Hero { param, enabled: true } => Control::knob(circle(paint::hero_circle(w.rect)), *param, &PARAMS[*param]).drag_range(220.0 * k),
                    Kind::Key { param, .. } | Kind::Toggle { param, .. } => Control::toggle(scaled(w.rect), *param, &PARAMS[*param]),
                    Kind::Step { param, names, key_w, hq: false, .. } => Control::key_bank(scaled(paint::step_rows(w.rect, *key_w, names.len())), *param, &PARAMS[*param], true),
                    Kind::Seg { param, hq: false, .. } => Control::key_bank(scaled(paint::seg_rect(w.rect)), *param, &PARAMS[*param], false),
                    Kind::Slider { param, enabled: true, .. } => {
                        let (x0, x1, _) = paint::slider_track(w.rect);
                        Control::fader_h(scaled((x0, w.rect.1, x1 - x0, w.rect.3)), *param, &PARAMS[*param])
                    }
                    _ => return None,
                };
                let name = PARAMS[control.param].name;
                Some(controls.add(control.tooltip(name)))
            })
            .collect();
        controls.set_modifiers(self.modifiers);
        for (param, value) in self.values.iter().enumerate() {
            controls.param_changed(param, *value);
        }
        self.controls = controls;
        self.signatures = vec![0; self.widgets.len()];
        self.hot = None;
        self.down = None;
        self.drag = None;
        self.damage.add_all();
    }

    /// Rebuild when the skin, the page or the selected input changed, but not
    /// while a control is held (its gesture would lose its end).
    fn follow(&mut self) {
        let source = self.values[ix::SOURCE].round().clamp(0.0, 2.0) as usize;
        if (self.wanted_page() != self.page || source != self.source) && self.controls.pressed().is_none() && self.drag.is_none() {
            let before = self.page.size();
            self.rebuild();
            if self.page.size() != before {
                self.ask_for_size();
            }
        }
    }

    /// Take the values the controls hold after input.
    fn sync(&mut self) {
        for control in self.controls.iter() {
            self.values[control.param] = control.value();
        }
        self.follow();
    }

    /// Set a parameter in one gesture.
    fn set(&mut self, param: usize, value: f32) {
        let value = PARAMS[param].clamp(value);
        self.host.set(param, value);
        self.values[param] = value;
        self.controls.param_changed(param, value);
    }

    fn looks(&self) -> Looks<'_> {
        Looks {
            values: &self.values,
            controls: &self.controls,
            ctl: &self.ctl,
            hot: self.hot,
            down: self.down,
            meters: &self.meters,
            settings: self.settings(),
            hq: self.hq,
            page: self.page,
            keys_active: self.keys_active,
            sample_rate: self.shared.as_ref().map_or(48_000.0, |s| s.sample_rate.get()),
            eq: &self.eq,
            vu: &self.vu,
        }
    }

    /// The custom part of a widget at a point in editor points: (widget, part).
    fn custom_at(&self, x: f32, y: f32) -> Option<(usize, usize)> {
        let (px, py) = self.to_panel(x, y);
        let looks = self.looks();
        self.widgets.iter().enumerate().rev().find_map(|(i, w)| paint::custom_hit(w, px, py, &looks).map(|part| (i, part)))
    }

    /// A press on the custom part of a widget.
    fn press(&mut self, i: usize, part: usize, x: f32, y: f32) {
        let settings = self.settings();
        match &self.widgets[i].kind {
            Kind::Route { param } => {
                let (param, now) = (*param, self.values[*param].round() as usize);
                self.set(param, if now == part + 1 { 0.0 } else { (part + 1) as f32 });
            }
            Kind::Step { param, names, hq, .. } | Kind::Seg { param, names, hq, .. } => {
                let (param, n, hq) = (*param, names.len(), *hq);
                let step = matches!(self.widgets[i].kind, Kind::Step { .. });
                let now = if hq && self.values[ix::HQ] > 0.5 { n - 1 } else { self.values[param].round() as usize };
                // A selector's key steps on; a lamp row or a segment picks its own option.
                let next = if !step { part } else if part == 0 { (now + 1) % n } else { part - 1 };
                if hq && next == n - 1 {
                    self.set(ix::HQ, 1.0);
                } else {
                    if hq {
                        self.set(ix::HQ, 0.0);
                    }
                    self.set(param, next as f32);
                }
            }
            Kind::PropStep { prop, names, .. } | Kind::PropSeg { prop, names } => {
                let step = matches!(self.widgets[i].kind, Kind::PropStep { .. });
                let now = match prop {
                    Prop::VuSource => usize::from(settings.vu_source),
                    Prop::VuCal => usize::from(settings.vu_high_cal),
                };
                let next = if !step { part } else if part == 0 { (now + 1) % names.len() } else { part - 1 };
                self.set_settings(match prop {
                    Prop::VuSource => EditorSettings { vu_source: next as u8, ..settings },
                    Prop::VuCal => EditorSettings { vu_high_cal: next == 1, ..settings },
                });
            }
            Kind::SkinKey => self.show(1, self.details),
            Kind::Button { action, .. } => match *action {
                Action::Skin(skin) => self.show(skin, self.details),
                Action::Page(page) => self.show(1, page == Page::Controls),
            },
            Kind::EqGraph | Kind::CompGraph if part != usize::MAX => {
                self.drag = Some(GraphDrag { widget: i, handle: part, open: Vec::new() });
                self.drag_to(x, y);
            }
            _ => {}
        }
    }

    /// Move the held graph handle to a point in editor points.
    fn drag_to(&mut self, x: f32, y: f32) {
        let Some(drag) = self.drag.take() else { return };
        let (px, py) = self.to_panel(x, y);
        let rect = self.widgets[drag.widget].rect;
        let edits = match self.widgets[drag.widget].kind {
            Kind::EqGraph => self.eq.drag(&graphs::eq_plot(rect), drag.handle, px, py, &self.values),
            _ => vec![CompCurve::new(&self.values).drag(&graphs::comp_plot(rect), drag.handle, px, py)],
        };
        let mut drag = drag;
        for (param, value) in edits {
            if !drag.open.contains(&param) {
                self.host.begin(param);
                drag.open.push(param);
            }
            let value = PARAMS[param].clamp(value);
            self.host.perform(param, value);
            self.values[param] = value;
            self.controls.param_changed(param, value);
        }
        self.drag = Some(drag);
    }

    fn end_drag(&mut self) {
        if let Some(drag) = self.drag.take() {
            for param in drag.open {
                self.host.end(param);
            }
        }
    }

    /// Mark the widgets whose look changed since the last time.
    fn collect_damage(&mut self) {
        let looks = self.looks();
        let changed: Vec<(usize, u64)> = self
            .widgets
            .iter()
            .enumerate()
            .filter_map(|(i, w)| {
                let signature = paint::signature(w, i, &looks);
                (signature != self.signatures[i]).then_some((i, signature))
            })
            .collect();
        for (i, signature) in changed {
            self.signatures[i] = signature;
            self.damage.add(DamageRect::from_tuple(paint::reach(&self.widgets[i])));
        }
        // The control set keeps its own record of what changed; the signatures cover it.
        let _ = self.controls.take_changed();
    }
}

impl Editor for BlueEditor {
    fn size(&self) -> (u32, u32) {
        self.size
    }

    fn size_range(&self) -> SizeRange {
        range(self.page)
    }

    fn resize(&mut self, width: u32, height: u32) {
        let size = range(self.page).clamp(width, height);
        if size == self.size {
            return;
        }
        self.size = size;
        // One scale for both skins: the window is the page's size times it.
        self.set_settings(EditorSettings { ui_scale: self.k(), ..self.settings() });
        self.rebuild();
    }

    fn bind(&mut self, state: Option<EditorState>) {
        self.shared = state.and_then(|s| s.downcast::<Shared>().ok());
        let Some(settings) = self.shared.as_ref().map(|s| s.settings()) else { return };
        self.local = settings;
        // Open in the skin and at the size the session was saved with.
        if self.wanted_page() != self.page {
            self.rebuild();
        }
        self.ask_for_size();
    }

    fn param_changed(&mut self, index: usize, plain: f32) {
        if index < COUNT {
            self.values[index] = plain;
            self.controls.param_changed(index, plain);
            self.follow();
        }
    }

    fn tick(&mut self, dt: f32) {
        self.follow();
        let Some(shared) = self.shared.clone() else { return };
        let settings = shared.settings();
        let channels = shared.channels.load(Relaxed).clamp(1, 2) as usize;
        let (mut in_peak, mut out_peak, mut in_square, mut out_square) = (0f32, 0f32, 0f32, 0f32);
        for ch in 0..channels {
            in_peak = in_peak.max(shared.input_peak[ch].take());
            out_peak = out_peak.max(shared.output_peak[ch].take());
            in_square = in_square.max(shared.input_square[ch].get());
            out_square = out_square.max(shared.output_square[ch].get());
        }
        let m = &mut self.meters;
        m.input_db = 20.0 * (in_peak + 1e-9).log10();
        m.output_db = 20.0 * (out_peak + 1e-9).log10();
        m.comp_gr = shared.comp_gr.take();
        m.gate_gr = shared.gate_gr.take();
        m.deess = (shared.deess_led.take() / 1.5).clamp(0.0, 1.0);
        if shared.overload.swap(false, Relaxed) {
            m.overload = 1.0;
        } else {
            m.overload = (m.overload - 0.05 * dt * 30.0).max(0.0);
        }
        m.key_input = shared.sidechain.load(Relaxed);
        let dbu = |square: f32| 20.0 * (square.sqrt() * FULL_SCALE_PEAK / DBU_VOLTS + 1e-9).log10();
        let cal = if settings.vu_high_cal { 18.0 } else { 4.0 };
        m.vu = match settings.vu_source {
            0 => dbu(in_square) - cal,
            1 => -m.comp_gr,
            _ => dbu(out_square) - cal,
        };
        // VU ballistics: about 300 ms to 99 %, whatever the tick rate.
        let step = 1.0 - (1.0f32 - 0.28).powf((dt * 30.0).clamp(0.0, 30.0));
        m.needle += (vu_position_for_db(m.vu) - m.needle) * step;
        m.read(dt);
        self.hq = shared.hq_status();
        if self.page == Page::Overview {
            self.eq.spectrum.update(&shared.spectrum);
        }
    }

    fn damage(&mut self) -> Option<Vec<DamageRect>> {
        self.collect_damage();
        self.damage.pending().map(|rects| rects.iter().map(|r| self.to_window((r.x, r.y, r.w, r.h))).collect())
    }

    fn render(&mut self, scale: f32) -> Frame<'_> {
        if self.page == Page::Overview {
            if let Some(view) = self.eq.view.as_mut() {
                view.update(&self.values);
            }
        }
        self.collect_damage();
        // The panel is drawn in its own units and the scene scaled to points, so the frame
        // is the editor's size in pixels at any window size (the canvas takes no scale
        // under one pixel per unit, which a small window on a 1x display needs).
        let (k, left, top) = self.fit();
        let device = if scale.is_finite() { scale.clamp(1.0, 4.0) } else { 2.0 };
        // Device pixels per panel unit: what knob sprites and textures are baked at.
        let unit = device * k;
        self.canvas.resize(self.size.0 as f32, self.size.1 as f32);
        if self.page == Page::Analogue {
            self.vu.prepare(unit);
        }
        let page = self.page;
        let looks = Looks {
            values: &self.values,
            controls: &self.controls,
            ctl: &self.ctl,
            hot: self.hot,
            down: self.down,
            meters: &self.meters,
            settings: self.shared.as_ref().map_or(self.local, |s| s.settings()),
            hq: self.hq,
            page,
            keys_active: self.keys_active,
            sample_rate: self.shared.as_ref().map_or(48_000.0, |s| s.sample_rate.get()),
            eq: &self.eq,
            vu: &self.vu,
        };
        let widgets = &self.widgets;
        // The canvas repaints everything when the damage is in many pieces or covers most of
        // the frame, and then every widget has to be in the scene. So the pieces are settled
        // here: few enough for the canvas to take as they are, or none (everything).
        let frame_px = ((self.size.0 as f32 * device).round() as usize, (self.size.1 as f32 * device).round() as usize);
        let corner = (left * device, top * device);
        let pieces = self.damage.pending().and_then(|rects| pieces(rects, unit, corner, frame_px));
        let damage: Option<Vec<DamageRect>> =
            pieces.as_ref().map(|px| px.iter().map(|r| DamageRect::new((r.0 as f32 - corner.0) / unit, (r.1 as f32 - corner.1) / unit, (r.2 - r.0) as f32 / unit, (r.3 - r.1) as f32 / unit)).collect());
        let touched = |w: &Widget| {
            let reach = DamageRect::from_tuple(paint::reach(w));
            damage.as_ref().is_none_or(|rects| rects.iter().any(|r| r.touches(&reach)))
        };
        let in_points: Option<Vec<DamageRect>> = pieces.as_ref().map(|px| px.iter().map(|r| DamageRect::new(r.0 as f32 / device, r.1 as f32 / device, (r.2 - r.0) as f32 / device, (r.3 - r.1) as f32 / device)).collect());
        let bake_at = |unit: f32| {
            draw::set_knob_scale(unit);
            hardware::set_device_scale(unit);
        };
        let frame = self.canvas.frame_damaged(
            device,
            page as u64,
            |dl| {
                bake_at(unit);
                let mut scene = Scene::new();
                paint::layer(&mut scene, page, looks.vu);
                paint::place(&mut scene, k, left, top);
                dl.append(scene);
            },
            |dl| {
                bake_at(unit);
                let mut scene = Scene::new();
                for (i, w) in widgets.iter().enumerate().filter(|(_, w)| touched(w)) {
                    paint::widget(&mut scene, w, i, &looks);
                }
                // Over all of them: the value of the knob in hand.
                for (i, w) in widgets.iter().enumerate().filter(|(_, w)| touched(w)) {
                    paint::overlay(&mut scene, w, i, &looks);
                }
                paint::place(&mut scene, k, left, top);
                dl.append(scene);
            },
            in_points.as_deref(),
        );
        self.damage.clear();
        frame
    }

    fn modifiers_changed(&mut self, modifiers: Modifiers) {
        self.modifiers = modifiers;
        self.controls.set_modifiers(modifiers);
    }

    fn mouse_down(&mut self, x: f32, y: f32, clicks: u32) {
        if let Some((i, part)) = self.custom_at(x, y) {
            self.controls.set_focus(None);
            self.down = Some((i, part));
            self.press(i, part, x, y);
            return;
        }
        self.controls.mouse_down(x, y, clicks);
        self.sync();
    }

    fn mouse_move(&mut self, x: f32, y: f32) {
        if self.drag.is_some() {
            self.drag_to(x, y);
            return;
        }
        self.hot = self.custom_at(x, y).filter(|h| h.1 != usize::MAX);
        self.controls.mouse_move(x, y);
        self.sync();
    }

    fn mouse_up(&mut self, x: f32, y: f32) {
        self.end_drag();
        self.down = None;
        self.controls.mouse_up(x, y);
        self.sync();
    }

    fn wheel(&mut self, x: f32, y: f32, dy: f32) {
        // Over a mid band's handle the wheel sets its Q.
        if let Some((i, handle)) = self.custom_at(x, y) {
            if matches!(self.widgets[i].kind, Kind::EqGraph) && (handle == 1 || handle == 2) {
                let param = if handle == 1 { ix::LMFQ } else { ix::HMFQ };
                self.set(param, self.values[param] + dy);
            }
            return;
        }
        self.controls.wheel(x, y, dy);
        self.sync();
    }

    fn wants_keys(&self) -> bool {
        self.controls.wants_keys()
    }

    fn key_down(&mut self, key: Key, modifiers: Modifiers) -> bool {
        let taken = self.controls.key_down(key, modifiers);
        self.sync();
        taken
    }

    fn key_up(&mut self, key: Key, modifiers: Modifiers) -> bool {
        let taken = self.controls.key_up(key, modifiers);
        self.sync();
        taken
    }

    fn focus_changed(&mut self, focused: bool) {
        self.keys_active = focused;
        if !focused {
            self.controls.release_keys();
        }
    }

    fn accessibility_nodes(&self) -> Vec<AccessNode> {
        self.controls.accessibility_nodes()
    }

    fn accessibility_action(&mut self, node: usize, action: AccessAction) -> bool {
        let done = self.controls.accessibility_action(node, action);
        self.sync();
        done
    }

    fn accessibility_range(&self, node: usize) -> Option<AccessRange> {
        self.controls.accessibility_range(node)
    }

    fn accessibility_set_value(&mut self, node: usize, value: f64) -> bool {
        let done = self.controls.accessibility_set_value(node, value);
        self.sync();
        done
    }

    fn accessibility_focus(&mut self, node: usize) -> bool {
        self.controls.accessibility_focus(node)
    }

    fn right_mouse_down(&mut self, x: f32, y: f32) -> bool {
        self.controls.context_click(x, y)
    }

    fn take_param_menu(&mut self) -> Option<ParamMenu> {
        self.controls.take_param_menu()
    }

    fn param_menu_action(&mut self, param: u32, action: ParamMenuAction) -> Option<String> {
        let result = self.controls.param_menu_action(param, action);
        self.sync();
        result
    }

    fn take_text_edit(&mut self) -> Option<TextEdit> {
        self.controls.take_text_edit()
    }

    fn text_edit_commit(&mut self, text: &str) -> bool {
        let done = self.controls.text_edit_commit(text);
        self.sync();
        done
    }

    fn text_edit_cancel(&mut self) {
        self.controls.text_edit_cancel();
    }

    fn tooltips(&self) -> Vec<Tooltip> {
        let mut tips = self.controls.tooltips();
        for w in &self.widgets {
            let text = match &w.kind {
                Kind::Route { .. } => "In: the section is in the audio path. Comp, Gate: it filters that key instead of the audio",
                Kind::SkinKey => "Switch between the analogue panel and the graphical Digital interface (display only)",
                Kind::HqLamp => "Full component solve is selected through Offline in All Controls. Playback uses Standard.",
                Kind::EqGraph => "Equaliser response: drag frequency and gain; scroll a mid band for Q",
                Kind::CompGraph => "Compressor detector target: drag threshold and ratio",
                _ => continue,
            };
            tips.push(Tooltip { rect: self.to_window(w.rect), text: text.to_string() });
        }
        tips
    }
}

#[cfg(test)]
mod tests;
