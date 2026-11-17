//! What the engine shares with the editor: meters, the analyser ring, and
//! the editor's own settings, which are saved with the session.

use std::sync::atomic::{AtomicBool, AtomicU32, Ordering::Relaxed};

use openplugin::dri::goodlookindri::spectrum::SampleRing;

use crate::engine::Telemetry;

/// An `f32` in an atomic.
#[derive(Default)]
pub struct AtomicF32(AtomicU32);

impl AtomicF32 {
    pub fn new(v: f32) -> Self {
        Self(AtomicU32::new(v.to_bits()))
    }
    pub fn get(&self) -> f32 {
        f32::from_bits(self.0.load(Relaxed))
    }
    pub fn set(&self, v: f32) {
        self.0.store(v.to_bits(), Relaxed);
    }
    /// Keep the larger of the stored value and `v` (both zero or more).
    pub fn raise(&self, v: f32) {
        // Non-negative floats order like their bits.
        self.0.fetch_max(v.max(0.0).to_bits(), Relaxed);
    }
    /// The value since the last take, which becomes zero.
    pub fn take(&self) -> f32 {
        f32::from_bits(self.0.swap(0, Relaxed))
    }
}

/// HQ Mode, as the editor reports it.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum HqStatus {
    Idle = 0,
    Armed,
    Solving,
    Failed,
    Unavailable,
}

impl HqStatus {
    pub fn text(self) -> &'static str {
        match self {
            Self::Armed => "Standard / full HQ on export",
            Self::Solving => "HQ: component solve",
            Self::Failed => "HQ failed: render muted",
            Self::Unavailable => "Offline HQ unavailable",
            Self::Idle => "Standard",
        }
    }
}

/// The editor's settings that are saved with the session.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct EditorSettings {
    /// 0 analogue, 1 Digital.
    pub skin: u8,
    /// 0 input, 1 compressor gain reduction, 2 output.
    pub vu_source: u8,
    /// 0 VU is +18 dBu (else +4 dBu).
    pub vu_high_cal: bool,
    /// The editor's size as a share of the shown page's panel (analogue 1560 x 450, Digital 1250 x 900).
    pub ui_scale: f32,
}

impl Default for EditorSettings {
    fn default() -> Self {
        Self { skin: 0, vu_source: 0, vu_high_cal: false, ui_scale: 1.0 }
    }
}

pub struct Shared {
    /// Peaks since the editor last took them (linear, full scale 1).
    pub input_peak: [AtomicF32; 2],
    pub output_peak: [AtomicF32; 2],
    /// Mean square over the last block.
    pub input_square: [AtomicF32; 2],
    pub output_square: [AtomicF32; 2],
    /// dB of reduction, largest since the editor last took it.
    pub comp_gr: AtomicF32,
    pub gate_gr: AtomicF32,
    pub limiter_gr: AtomicF32,
    pub deess_led: AtomicF32,
    pub overload: AtomicBool,
    pub channels: AtomicU32,
    pub sidechain: AtomicBool,
    pub hq_status: AtomicU32,
    pub sample_rate: AtomicF32,
    /// The left output, for the Digital skin's analyser.
    pub spectrum: SampleRing,
    skin: AtomicU32,
    vu_source: AtomicU32,
    vu_high_cal: AtomicBool,
    ui_scale: AtomicF32,
}

impl Default for Shared {
    fn default() -> Self {
        let s = Self {
            input_peak: Default::default(),
            output_peak: Default::default(),
            input_square: Default::default(),
            output_square: Default::default(),
            comp_gr: AtomicF32::default(),
            gate_gr: AtomicF32::default(),
            limiter_gr: AtomicF32::default(),
            deess_led: AtomicF32::default(),
            overload: AtomicBool::new(false),
            channels: AtomicU32::new(2),
            sidechain: AtomicBool::new(false),
            hq_status: AtomicU32::new(0),
            sample_rate: AtomicF32::new(48_000.0),
            spectrum: SampleRing::new(),
            skin: AtomicU32::new(0),
            vu_source: AtomicU32::new(0),
            vu_high_cal: AtomicBool::new(false),
            ui_scale: AtomicF32::default(),
        };
        s.set_settings(EditorSettings::default());
        s
    }
}

impl Shared {
    /// One block's measurements, from the audio thread.
    pub fn publish(&self, t: &Telemetry, channels: usize, sidechain: bool) {
        for ch in 0..2 {
            self.input_peak[ch].raise(t.input_peak[ch]);
            self.output_peak[ch].raise(t.output_peak[ch]);
            self.input_square[ch].set(t.input_square[ch]);
            self.output_square[ch].set(t.output_square[ch]);
        }
        self.comp_gr.raise(t.comp_gr);
        self.gate_gr.raise(t.gate_gr);
        self.limiter_gr.raise(t.limiter_gr);
        self.deess_led.raise(t.deess_led);
        if t.overload != 0 {
            self.overload.store(true, Relaxed);
        }
        self.channels.store(channels as u32, Relaxed);
        self.sidechain.store(sidechain, Relaxed);
        self.hq_status.store(t.hq_status.clamp(0, 4) as u32, Relaxed);
    }

    pub fn hq_status(&self) -> HqStatus {
        match self.hq_status.load(Relaxed) {
            1 => HqStatus::Armed,
            2 => HqStatus::Solving,
            3 => HqStatus::Failed,
            4 => HqStatus::Unavailable,
            _ => HqStatus::Idle,
        }
    }

    pub fn set_hq_status(&self, status: HqStatus) {
        self.hq_status.store(status as u32, Relaxed);
    }

    pub fn settings(&self) -> EditorSettings {
        EditorSettings {
            skin: self.skin.load(Relaxed).min(1) as u8,
            vu_source: self.vu_source.load(Relaxed).min(2) as u8,
            vu_high_cal: self.vu_high_cal.load(Relaxed),
            ui_scale: self.ui_scale.get(),
        }
    }

    pub fn set_settings(&self, s: EditorSettings) {
        self.skin.store(u32::from(s.skin.min(1)), Relaxed);
        self.vu_source.store(u32::from(s.vu_source.min(2)), Relaxed);
        self.vu_high_cal.store(s.vu_high_cal, Relaxed);
        self.ui_scale.set(if s.ui_scale.is_finite() { s.ui_scale.clamp(0.5, 2.0) } else { 1.0 });
    }
}
