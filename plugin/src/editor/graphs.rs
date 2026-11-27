//! The Digital skin's two graphs: the equaliser's response over the output
//! spectrum, and the compressor's detector target. Geometry and editing rules
//! are those of the 1.0 release (Source/UI/DigitalGraphs.h at tag v1.0.0).

use openplugin::dri::goodlookindri::spectrum::{RealFft, SampleRing};

use crate::engine::{EqView, CURVE_POINTS};
use crate::params::index as ix;
use crate::COUNT;

const FFT: usize = 2048;

/// The output spectrum, smoothed between frames.
pub struct Spectrum {
    fft: RealFft,
    window: Vec<f32>,
    input: Vec<f32>,
    re: Vec<f32>,
    im: Vec<f32>,
    /// dB per bin, `FFT / 2` of them.
    pub db: Vec<f32>,
}

impl Default for Spectrum {
    fn default() -> Self {
        let window = (0..FFT).map(|i| 0.5 - 0.5 * (std::f32::consts::TAU * i as f32 / FFT as f32).cos()).collect();
        Self { fft: RealFft::new(FFT), window, input: vec![0.0; FFT], re: vec![0.0; FFT / 2 + 1], im: vec![0.0; FFT / 2 + 1], db: vec![-100.0; FFT / 2] }
    }
}

impl Spectrum {
    pub const SIZE: usize = FFT;

    pub fn update(&mut self, ring: &SampleRing) {
        if !ring.copy_latest(&mut self.input) {
            self.input.fill(0.0);
        }
        for (s, w) in self.input.iter_mut().zip(&self.window) {
            *s *= w;
        }
        self.fft.forward(&self.input, &mut self.re, &mut self.im);
        for (i, db) in self.db.iter_mut().enumerate() {
            let magnitude = (self.re[i] * self.re[i] + self.im[i] * self.im[i]).sqrt();
            let target = 20.0 * (magnitude * 4.0 / FFT as f32).max(1e-9).log10();
            *db += (target - *db) * if target > *db { 0.8 } else { 0.2 };
        }
    }
}

/// The equaliser graph in a rectangle `(x, y, w, h)`.
pub struct EqGraph {
    pub view: Option<EqView>,
    pub spectrum: Spectrum,
}

impl Default for EqGraph {
    fn default() -> Self {
        Self { view: EqView::new(), spectrum: Spectrum::default() }
    }
}

#[derive(Clone, Copy)]
pub struct Plot {
    pub x: f32,
    pub y: f32,
    pub w: f32,
    pub h: f32,
}

impl Plot {
    pub fn right(&self) -> f32 {
        self.x + self.w
    }
    pub fn bottom(&self) -> f32 {
        self.y + self.h
    }
}

/// The plot area of the equaliser graph inside its rectangle.
pub fn eq_plot(r: (f32, f32, f32, f32)) -> Plot {
    Plot { x: r.0 + 40.0, y: r.1 + 28.0, w: r.2 - 60.0, h: r.3 - 56.0 }
}

pub fn eq_x(a: &Plot, hz: f32) -> f32 {
    a.x + (hz / 20.0).ln() / 1000f32.ln() * a.w
}
pub fn eq_y(a: &Plot, db: f32) -> f32 {
    a.y + a.h * 0.5 - db / 24.0 * a.h * 0.5
}
pub fn eq_hz(a: &Plot, px: f32) -> f32 {
    20.0 * 1000f32.powf((px - a.x) / a.w)
}
pub fn eq_db(a: &Plot, py: f32) -> f32 {
    (a.y + a.h * 0.5 - py) / a.h * 48.0
}

pub const SHELF_LOW: [[f32; 4]; 2] = [[20.0, 56.0, 160.0, 460.0], [33.0, 95.0, 270.0, 655.0]];
pub const SHELF_HIGH: [[f32; 4]; 2] = [[1500.0, 3300.0, 6800.0, 15000.0], [2200.0, 4700.0, 10000.0, 18000.0]];

impl EqGraph {
    /// Where handle `i` is drawn.
    pub fn handle(&self, a: &Plot, i: usize) -> (f32, f32) {
        let [hz, db] = self.view.as_ref().map_or([1000.0, 0.0], |v| v.handles[i]);
        let x = eq_x(a, hz);
        (if i >= 4 { x.clamp(a.x, a.right()) } else { x }, eq_y(a, db))
    }

    /// The handle within reach of a point, nearest first.
    pub fn pick(&self, a: &Plot, x: f32, y: f32) -> Option<usize> {
        let mut best = (22.0f32, None);
        for i in 0..6 {
            let (hx, hy) = self.handle(a, i);
            let d = ((hx - x).powi(2) + (hy - y).powi(2)).sqrt();
            if d < best.0 {
                best = (d, Some(i));
            }
        }
        best.1
    }

    /// The parameters a drag of handle `i` to a point sets, as (index, plain value).
    pub fn drag(&self, a: &Plot, i: usize, x: f32, y: f32, values: &[f32; COUNT]) -> Vec<(usize, f32)> {
        let hz = eq_hz(a, x).clamp(20.0, 20_000.0);
        let gain = eq_db(a, y);
        match i {
            4 => vec![(ix::HPF, hz)],
            5 => vec![(ix::LPF, hz)],
            1 => vec![(ix::LMFGAIN, gain), (ix::LMFFREQ, hz / if values[ix::LMFX3] > 0.5 { 3.0 } else { 1.0 })],
            2 => vec![(ix::HMFGAIN, gain), (ix::HMFFREQ, hz / if values[ix::HMFX3] > 0.5 { 3.0 } else { 1.0 })],
            _ => {
                let low = i == 0;
                let high = values[if low { ix::LFHIGH } else { ix::HFHIGH }] > 0.5;
                let table = if low { &SHELF_LOW } else { &SHELF_HIGH }[usize::from(high)];
                let nearest = (0..4).min_by(|a, b| (hz / table[*a]).ln().abs().total_cmp(&(hz / table[*b]).ln().abs())).unwrap_or(0);
                vec![(if low { ix::LFGAIN } else { ix::HFGAIN }, gain), (if low { ix::LFFREQ } else { ix::HFFREQ }, nearest as f32)]
            }
        }
    }

    pub fn curve(&self) -> Option<(&[f32; CURVE_POINTS], &[f32; CURVE_POINTS])> {
        self.view.as_ref().map(|v| (&v.curve, &v.filter_curve))
    }
}

/// Soft-knee downward compression: dB of reduction for an input level
/// (Source/Dynamics/Detectors.h, `compressionDb`).
pub fn compression_db(level: f32, threshold: f32, ratio: f32, knee: f32) -> f32 {
    let over = level - threshold;
    let slope = 1.0 - 1.0 / ratio.max(1.0);
    if knee > 0.0 && over.abs() <= knee * 0.5 {
        let t = over + knee * 0.5;
        return slope * t * t / (2.0 * knee);
    }
    if over > 0.0 { slope * over } else { 0.0 }
}

/// The compressor graph: input and output level from -40 to +20 dBu.
pub fn comp_plot(r: (f32, f32, f32, f32)) -> Plot {
    Plot { x: r.0 + 38.0, y: r.1 + 28.0, w: r.2 - 54.0, h: r.3 - 56.0 }
}
pub fn comp_x(a: &Plot, db: f32) -> f32 {
    a.x + (db + 40.0) / 60.0 * a.w
}
pub fn comp_y(a: &Plot, db: f32) -> f32 {
    a.bottom() - (db + 40.0) / 60.0 * a.h
}
fn comp_level(a: &Plot, py: f32) -> f32 {
    (a.bottom() - py) / a.h * 60.0 - 40.0
}

/// The compressor's static curve for the current settings.
pub struct CompCurve {
    kind: usize,
    threshold: f32,
    ratio: f32,
    makeup: f32,
}

impl CompCurve {
    pub fn new(values: &[f32; COUNT]) -> Self {
        Self { kind: values[ix::COMPTYPE].round() as usize, threshold: values[ix::THRESHOLD], ratio: values[ix::RATIO], makeup: values[ix::MAKEUP] }
    }

    /// The threshold the curve bends at (the limiter's sits 4 dB higher).
    fn knee_at(&self) -> f32 {
        self.threshold + if self.kind == 2 { 4.0 } else { 0.0 }
    }

    pub fn output(&self, input: f32) -> f32 {
        let (ratio, knee) = match self.kind {
            0 => (self.ratio, 10.0),
            2 => (5.0 + (self.ratio - 1.5) / 8.5 * 15.0, 2.0),
            _ => (1.5 + (self.ratio - 1.5) / 8.5 * 3.5, 12.0),
        };
        input - compression_db(input, self.knee_at(), ratio, knee) + self.makeup
    }

    /// Threshold, ratio and make-up handles.
    pub fn handle(&self, a: &Plot, i: usize) -> (f32, f32) {
        let t = self.knee_at();
        let input = match i {
            0 => t,
            1 => (t + 18.0).min(18.0),
            _ => (t - 14.0).max(-38.0),
        };
        (comp_x(a, input), comp_y(a, self.output(input)))
    }

    pub fn pick(&self, a: &Plot, x: f32, y: f32) -> Option<usize> {
        (0..3).rev().find(|i| {
            let (hx, hy) = self.handle(a, *i);
            ((hx - x).powi(2) + (hy - y).powi(2)).sqrt() < 20.0
        })
    }

    /// The parameter a drag of handle `i` to a point sets.
    pub fn drag(&self, a: &Plot, i: usize, x: f32, y: f32) -> (usize, f32) {
        let t = self.knee_at();
        match i {
            0 => (ix::THRESHOLD, (x - a.x) / a.w * 60.0 - 40.0 - if self.kind == 2 { 4.0 } else { 0.0 }),
            1 => {
                let run = (t + 18.0).min(18.0) - t;
                let rise = comp_level(a, y) - self.makeup - t;
                let ratio = run / rise.max(0.1);
                (
                    ix::RATIO,
                    match self.kind {
                        0 => ratio,
                        2 => 1.5 + (ratio - 5.0) / 15.0 * 8.5,
                        _ => 1.5 + (ratio - 1.5) / 3.5 * 8.5,
                    },
                )
            }
            _ => (ix::MAKEUP, self.makeup + comp_level(a, y) - self.output((t - 14.0).max(-38.0))),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::PARAMS;

    fn defaults() -> [f32; COUNT] {
        std::array::from_fn(|i| PARAMS[i].default)
    }

    #[test]
    fn the_equaliser_curve_is_flat_until_a_section_is_in_and_then_follows_it() {
        let mut graph = EqGraph::default();
        let mut values = defaults();
        let view = graph.view.as_mut().expect("the model's stages load");
        assert!(view.update(&values));
        assert!(view.curve.iter().all(|db| db.abs() < 1e-3), "every section is out");
        assert!(!view.update(&values), "nothing changed");
        values[ix::SHELFROUTE] = 1.0;
        values[ix::LFGAIN] = 12.0;
        assert!(view.update(&values));
        assert!(view.curve[0] > 8.0 && view.curve[CURVE_POINTS - 1].abs() < 1.0, "{} {}", view.curve[0], view.curve[CURVE_POINTS - 1]);
        assert_eq!(view.handles[0], [56.0, 12.0]);
        // A compressor edit does not recompute the networks.
        values[ix::RATIO] = 5.0;
        assert!(!view.update(&values));
    }

    #[test]
    fn dragging_a_handle_sets_the_parameters_the_release_set() {
        let graph = EqGraph::default();
        let a = eq_plot((0.0, 0.0, 656.0, 242.0));
        let values = defaults();
        let at = |hz: f32, db: f32| (eq_x(&a, hz), eq_y(&a, db));
        let (x, y) = at(70.0, 6.0);
        let edits = graph.drag(&a, 0, x, y, &values);
        assert_eq!(edits[1], (ix::LFFREQ, 1.0), "56 Hz is the nearest low shelf to 70 Hz");
        assert!((edits[0].1 - 6.0).abs() < 1e-3);
        let (x, y) = at(3000.0, -4.0);
        let edits = graph.drag(&a, 2, x, y, &values);
        assert!((edits[1].1 - 3000.0).abs() < 1.0 && (edits[0].1 + 4.0).abs() < 1e-3);
        assert_eq!(graph.drag(&a, 4, at(80.0, 0.0).0, 0.0, &values)[0].0, ix::HPF);
    }

    #[test]
    fn the_compressor_curve_and_its_handles_agree() {
        let mut values = defaults();
        values[ix::THRESHOLD] = -10.0;
        values[ix::RATIO] = 4.0;
        values[ix::MAKEUP] = 3.0;
        let curve = CompCurve::new(&values);
        assert!((curve.output(-40.0) - (-37.0)).abs() < 1e-4, "below the knee only the make-up acts");
        assert!((curve.output(10.0) - (10.0 - 0.75 * 20.0 + 3.0)).abs() < 1e-4, "above the knee the slope is 1 - 1/ratio");
        let a = comp_plot((0.0, 0.0, 296.0, 178.0));
        // Dragging each handle to where it already is changes nothing.
        for (i, (param, was)) in [(ix::THRESHOLD, -10.0), (ix::RATIO, 4.0), (ix::MAKEUP, 3.0)].into_iter().enumerate() {
            let (x, y) = curve.handle(&a, i);
            let (p, v) = curve.drag(&a, i, x, y);
            assert_eq!(p, param);
            if i != 1 {
                assert!((v - was).abs() < 1e-3, "{i}: {v}");
            }
        }
        assert_eq!(compression_db(-20.0, 0.0, 4.0, 10.0), 0.0);
    }
}
