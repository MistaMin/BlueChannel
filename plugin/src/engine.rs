//! The C++ engine (Source/Model) behind the C ABI of cpp/bridge.h.

use std::ptr::NonNull;

use crate::params::COUNT;

/// What the engine measured during one block (`BcTelemetry`).
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct Telemetry {
    pub input_peak: [f32; 2],
    pub output_peak: [f32; 2],
    pub input_square: [f32; 2],
    pub output_square: [f32; 2],
    pub comp_gr: f32,
    pub gate_gr: f32,
    pub limiter_gr: f32,
    pub deess_led: f32,
    pub overload: i32,
    pub active_factor: i32,
    pub rendering: i32,
    pub hq_status: i32,
}

#[repr(C)]
struct BcEngine {
    _opaque: [u8; 0],
}

extern "C" {
    fn bc_create() -> *mut BcEngine;
    fn bc_destroy(engine: *mut BcEngine);
    fn bc_prepare(engine: *mut BcEngine, sample_rate: f64, max_block: i32, channels: i32) -> i32;
    fn bc_reset(engine: *mut BcEngine);
    #[allow(clippy::too_many_arguments)]
    fn bc_process(
        engine: *mut BcEngine,
        io: *const *mut f32,
        channels: i32,
        frames: i32,
        key: *const *const f32,
        key_channels: i32,
        values: *const f32,
        offline: i32,
        out: *mut Telemetry,
    );
    fn bc_param_count() -> i32;
    fn bc_hq_compiled() -> i32;
    fn bc_hq_error(engine: *mut BcEngine) -> *const std::ffi::c_char;
    fn bc_eq_create() -> *mut BcEqView;
    fn bc_eq_destroy(view: *mut BcEqView);
    fn bc_eq_update(view: *mut BcEqView, values: *const f32, curve: *mut f32, filter_curve: *mut f32, handles: *mut f32) -> i32;
}

#[repr(C)]
struct BcEqView {
    _opaque: [u8; 0],
}

pub const CURVE_POINTS: usize = 161;

/// The equaliser's analogue response, for the editor's graph (the model's
/// own stages, on the editor's thread).
pub struct EqView {
    raw: NonNull<BcEqView>,
    /// dB of the sections in the audio path, 161 log-spaced points from 20 Hz to 20 kHz.
    pub curve: [f32; CURVE_POINTS],
    /// The filters alone.
    pub filter_curve: [f32; CURVE_POINTS],
    /// (Hz, dB) of the LF, LMF, HMF, HF, HP and LP handles.
    pub handles: [[f32; 2]; 6],
}

impl EqView {
    pub fn new() -> Option<Self> {
        NonNull::new(unsafe { bc_eq_create() }).map(|raw| Self { raw, curve: [0.0; CURVE_POINTS], filter_curve: [0.0; CURVE_POINTS], handles: [[0.0; 2]; 6] })
    }

    /// True when the curves changed.
    pub fn update(&mut self, values: &[f32; COUNT]) -> bool {
        unsafe { bc_eq_update(self.raw.as_ptr(), values.as_ptr(), self.curve.as_mut_ptr(), self.filter_curve.as_mut_ptr(), self.handles.as_mut_ptr().cast()) == 1 }
    }
}

impl Drop for EqView {
    fn drop(&mut self) {
        unsafe { bc_eq_destroy(self.raw.as_ptr()) }
    }
}

pub struct Engine {
    raw: NonNull<BcEngine>,
    prepared: bool,
}

// The engine has no thread affinity, no thread-local state and no globals
// shared between instances that are written after load.
unsafe impl Send for Engine {}

impl Engine {
    /// `None` when the engine could not be allocated.
    pub fn new() -> Option<Self> {
        assert_eq!(unsafe { bc_param_count() }, COUNT as i32, "cpp/bridge.cpp and src/params.rs disagree on the parameter table");
        NonNull::new(unsafe { bc_create() }).map(|raw| Self { raw, prepared: false })
    }

    /// Allocates; not for the audio thread.
    pub fn prepare(&mut self, sample_rate: f64, max_block: usize, channels: usize) {
        self.prepared = unsafe { bc_prepare(self.raw.as_ptr(), sample_rate, max_block.min(i32::MAX as usize) as i32, channels as i32) } == 1;
    }

    /// Whether the offline component solver is in this build.
    pub fn hq_compiled() -> bool {
        unsafe { bc_hq_compiled() == 1 }
    }

    /// Why the last HQ block failed.
    pub fn hq_error(&mut self) -> String {
        unsafe { std::ffi::CStr::from_ptr(bc_hq_error(self.raw.as_ptr())) }.to_string_lossy().into_owned()
    }

    pub fn reset(&mut self) {
        if self.prepared {
            unsafe { bc_reset(self.raw.as_ptr()) }
        }
    }

    /// In place over up to two channels of `io`; `key` is the external key (up
    /// to two channels). Leaves the audio alone until `prepare` has succeeded.
    pub fn process(&mut self, io: &mut [&mut [f32]], key: Option<&[&[f32]]>, frames: usize, values: &[f32; COUNT], offline: bool) -> Telemetry {
        let mut telemetry = Telemetry::default();
        let channels = io.len().min(2);
        if !self.prepared || channels == 0 || frames == 0 || io[..channels].iter().any(|c| c.len() < frames) {
            return telemetry;
        }
        let mut io_ptrs = [std::ptr::null_mut::<f32>(); 2];
        for (ptr, channel) in io_ptrs.iter_mut().zip(io.iter_mut()) {
            *ptr = channel.as_mut_ptr();
        }
        let mut key_ptrs = [std::ptr::null::<f32>(); 2];
        let mut key_channels = 0;
        for (ptr, channel) in key_ptrs.iter_mut().zip(key.unwrap_or(&[])) {
            if channel.len() < frames {
                break;
            }
            *ptr = channel.as_ptr();
            key_channels += 1;
        }
        unsafe {
            bc_process(
                self.raw.as_ptr(),
                io_ptrs.as_ptr(),
                channels as i32,
                frames as i32,
                key_ptrs.as_ptr(),
                key_channels,
                values.as_ptr(),
                i32::from(offline),
                &mut telemetry,
            );
        }
        telemetry
    }
}

impl Drop for Engine {
    fn drop(&mut self) {
        unsafe { bc_destroy(self.raw.as_ptr()) }
    }
}
