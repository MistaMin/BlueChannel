//! Blue Channel on OpenPluginSDK.
//!
//! The signal path is the C++ model of the JUCE release (../Source/Model,
//! Circuit, DSP, Dynamics; no JUCE in it), called through cpp/bridge.cpp. This
//! crate is the plugin layer: parameters, identity, state and buses.

mod editor;
mod engine;
mod params;
mod programs;
pub mod shared;
pub mod state_import;

use std::sync::Arc;

use openplugin::core::{decode_params, encode_params, AaxIds, AuIds, Vst3Ids};
use openplugin::prelude::*;

pub use editor::{make_editor, BlueEditor, Page};
pub use engine::Telemetry;
pub use params::{COUNT, INFO, PARAMS};
pub use shared::{EditorSettings, HqStatus, Shared};

static LAYOUTS: [IoConfig; 4] = [
    IoConfig { input: SpeakerLayout::Stereo, output: SpeakerLayout::Stereo, sidechain: Some(SpeakerLayout::Stereo) },
    IoConfig { input: SpeakerLayout::Stereo, output: SpeakerLayout::Stereo, sidechain: None },
    IoConfig { input: SpeakerLayout::Mono, output: SpeakerLayout::Mono, sidechain: Some(SpeakerLayout::Mono) },
    IoConfig { input: SpeakerLayout::Mono, output: SpeakerLayout::Mono, sidechain: None },
];

fn defaults() -> [f32; COUNT] {
    std::array::from_fn(|i| PARAMS[i].default)
}

pub struct BlueChannel {
    /// Each parameter's plain value, in table order.
    values: [f32; COUNT],
    engine: Option<engine::Engine>,
    offline: bool,
    sample_rate: f32,
    telemetry: Telemetry,
    shared: Arc<Shared>,
}

/// How long the output can still be above -90 dBFS after the input stops: the
/// model's coupling capacitors and transformers settling after a loud signal.
/// Measured with examples/tail.rs: 0.12 s at the defaults, 2.9 s for an
/// overdriven instrument input, 4.2 s for the mic input at full gain. The JUCE
/// release reported no tail.
const TAIL_SECONDS: f32 = 5.0;

impl Default for BlueChannel {
    fn default() -> Self {
        Self { values: defaults(), engine: engine::Engine::new(), offline: false, sample_rate: 48_000.0, telemetry: Telemetry::default(), shared: Arc::new(Shared::default()) }
    }
}

impl BlueChannel {
    fn index(id: &str) -> Option<usize> {
        PARAMS.iter().position(|p| p.id == id)
    }

    /// Each parameter's plain value, in table order.
    pub fn values(&self) -> &[f32] {
        &self.values
    }

    /// The editor's settings, which are saved with the session.
    pub fn editor_settings(&self) -> EditorSettings {
        self.shared.settings()
    }

    pub fn set_editor_settings(&self, settings: EditorSettings) {
        self.shared.set_settings(settings);
    }

    /// Whether HQ Mode's offline component solver is in this build.
    pub fn hq_compiled() -> bool {
        engine::Engine::hq_compiled()
    }

    /// HQ Mode's state after the last block, and why it failed if it did.
    pub fn hq_status(&mut self) -> (HqStatus, String) {
        let status = self.shared.hq_status();
        let error = if status == HqStatus::Failed { self.engine.as_mut().map_or_else(String::new, engine::Engine::hq_error) } else { String::new() };
        (status, error)
    }

    /// What the engine measured during the last block.
    pub fn telemetry(&self) -> &Telemetry {
        &self.telemetry
    }
}

fn db(linear: f32) -> f32 {
    20.0 * linear.max(1e-6).log10()
}

impl Plugin for BlueChannel {
    fn info(&self) -> &'static PluginInfo {
        &INFO
    }

    fn prepare(&mut self, sample_rate: f32, max_block: usize, channels: usize) {
        self.sample_rate = sample_rate;
        self.shared.sample_rate.set(sample_rate);
        if let Some(engine) = &mut self.engine {
            engine.prepare(f64::from(sample_rate), max_block, channels.clamp(1, 2));
        }
    }

    fn set_offline(&mut self, offline: bool) {
        self.offline = offline;
    }

    fn reset(&mut self) {
        if let Some(engine) = &mut self.engine {
            engine.reset();
        }
    }

    fn set_param(&mut self, id: &str, value: f32) -> bool {
        Self::index(id).is_some_and(|i| self.set_param_index(i, value))
    }

    fn set_param_index(&mut self, index: usize, value: f32) -> bool {
        match PARAMS.get(index) {
            Some(info) => {
                self.values[index] = info.clamp(value);
                true
            }
            None => false,
        }
    }

    fn param(&self, id: &str) -> Option<f32> {
        Self::index(id).map(|i| self.values[i])
    }

    fn process(&mut self, io: &mut [&mut [f32]], frames: usize) {
        self.process_buses(io, None, frames);
    }

    fn process_buses(&mut self, io: &mut [&mut [f32]], sidechain: Option<&[&[f32]]>, frames: usize) {
        let Some(engine) = &mut self.engine else { return };
        // One NaN would stay in the model's state for good: non-finite input is silence,
        // and a key that carries one is left out of the block.
        for channel in io.iter_mut().take(2) {
            for sample in channel.iter_mut().take(frames) {
                if !sample.is_finite() {
                    *sample = 0.0;
                }
            }
        }
        let key = sidechain.filter(|key| key.iter().take(2).all(|c| c.iter().take(frames).all(|s| s.is_finite())));
        self.telemetry = engine.process(io, key, frames, &self.values, self.offline);
        self.shared.publish(&self.telemetry, io.len().min(2), sidechain.is_some_and(|key| !key.is_empty()));
        if let Some(left) = io.first() {
            self.shared.spectrum.push(&[&left[..frames.min(left.len())]]);
        }
    }

    fn editor_state(&self) -> Option<EditorState> {
        Some(self.shared.clone())
    }

    fn preset_count(&self) -> usize {
        programs::PROGRAMS.len()
    }

    fn preset_name(&self, index: usize) -> &'static str {
        programs::PROGRAMS.get(index).map_or("", |p| p.name)
    }

    fn preset_values(&self, index: usize) -> Option<&'static [f32]> {
        programs::values(index)
    }

    fn tail(&self) -> Tail {
        Tail::from_samples((TAIL_SECONDS * self.sample_rate) as usize)
    }

    fn meters(&self) -> MeterFrame {
        let t = &self.telemetry;
        let mut m = MeterFrame::SILENT;
        m.input_db = t.input_peak.map(db);
        m.output_db = t.output_peak.map(db);
        m.reduction_db = t.comp_gr.max(t.gate_gr).max(t.limiter_gr);
        m.clip = u8::from(t.overload != 0);
        m
    }

    fn supported_layouts(&self) -> &'static [IoConfig] {
        &LAYOUTS
    }

    fn note_ports(&self) -> NotePorts {
        NotePorts { input: false, output: false }
    }

    // The ids of the 1.0.0 JUCE release, so its projects find this plugin.
    #[cfg(not(feature = "qa-identity"))]
    fn vst3_ids(&self) -> Option<Vst3Ids> {
        Vst3Ids::from_hex("ABCDEF019182FAEB4F704772426C4368", "ABCDEF011234ABCD4F704772426C4368")
    }

    #[cfg(not(feature = "qa-identity"))]
    fn au_ids(&self) -> Option<AuIds> {
        Some(AuIds { subtype: *b"BlCh", manufacturer: *b"OpGr" })
    }

    #[cfg(not(feature = "qa-identity"))]
    fn lv2_uri(&self) -> Option<&'static str> {
        Some("urn:opengrid:bluechannel")
    }

    // The JUCE build's AAX codes: its manufacturer and plugin codes, and the plug-in type ids
    // JUCE derives from the bus layout (`jcaa` plus 0x101 for mono, 0x202 for stereo).
    #[cfg(not(feature = "qa-identity"))]
    fn aax_ids(&self) -> Option<AaxIds> {
        Some(AaxIds { manufacturer: *b"OpGr", product: *b"BlCh", mono: *b"jcbb", stereo: *b"jccc" })
    }

    /// `BCH1`, the editor's settings (skin, meter source, 0 VU level, a spare
    /// byte, the size as `f32`), then the SDK's parameter snapshot.
    fn save_state(&mut self) -> Option<Vec<u8>> {
        let e = self.shared.settings();
        let mut out = STATE_MAGIC.to_vec();
        out.extend_from_slice(&[e.skin, e.vu_source, u8::from(e.vu_high_cal), 0]);
        out.extend_from_slice(&e.ui_scale.to_le_bytes());
        out.extend_from_slice(&encode_params(&INFO, &self.values));
        Some(out)
    }

    /// The JUCE release's state first (sessions and presets made before the
    /// port), then the port's own. A refused state changes nothing.
    fn load_state(&mut self, data: &[u8]) -> bool {
        let (values, settings) = match state_import::import_original(data) {
            Some(Ok(state)) => state,
            Some(Err(_)) => return false,
            None => match decode_own(data) {
                Some(state) => state,
                None => return false,
            },
        };
        self.values = values;
        self.shared.set_settings(settings);
        true
    }
}

const STATE_MAGIC: &[u8; 4] = b"BCH1";

fn decode_own(data: &[u8]) -> Option<([f32; COUNT], EditorSettings)> {
    let body = data.strip_prefix(STATE_MAGIC)?;
    let (head, params) = body.split_at_checked(8)?;
    let ui_scale = f32::from_le_bytes([head[4], head[5], head[6], head[7]]);
    if head[0] > 1 || head[1] > 2 || head[2] > 1 || head[3] != 0 || !(0.5..=2.0).contains(&ui_scale) {
        return None;
    }
    let values = <[f32; COUNT]>::try_from(decode_params(&INFO, params)?).ok()?;
    Some((values, EditorSettings { skin: head[0], vu_source: head[1], vu_high_cal: head[2] == 1, ui_scale }))
}

openplugin::targets::export_clap!(BlueChannel, editor = make_editor);
openplugin::targets::export_lv2!(BlueChannel, editor = make_editor);
openplugin::targets::export_shell!(BlueChannel, editor = make_editor);

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn own_state_round_trips() {
        let mut a = BlueChannel::default();
        for p in PARAMS.iter() {
            assert!(a.set_param(p.id, p.max));
        }
        let blob = a.save_state().expect("a state");
        let mut b = BlueChannel::default();
        assert!(b.load_state(&blob));
        assert_eq!(a.values(), b.values());
    }

    #[test]
    fn the_editor_settings_are_saved_with_the_state() {
        let mut a = BlueChannel::default();
        let settings = EditorSettings { skin: 1, vu_source: 2, vu_high_cal: true, ui_scale: 1.25 };
        a.set_editor_settings(settings);
        let blob = a.save_state().unwrap();
        let mut b = BlueChannel::default();
        assert!(b.load_state(&blob));
        assert_eq!(b.editor_settings(), settings);
        assert_eq!(b.save_state().unwrap(), blob);
    }

    #[test]
    fn the_factory_programs_are_those_of_the_release() {
        let p = BlueChannel::default();
        let names: Vec<_> = (0..p.preset_count()).map(|i| p.preset_name(i)).collect();
        assert_eq!(names, ["Init", "Vocal", "Drum Bus", "Bass DI", "Gated Toms", "Mix Polish"]);
        let init = p.preset_values(0).unwrap();
        assert!(init.iter().zip(PARAMS.iter()).all(|(v, info)| *v == info.default));
        let vocal = p.preset_values(1).unwrap();
        let at = |id: &str| vocal[PARAMS.iter().position(|p| p.id == id).unwrap()];
        assert_eq!((at("hpf"), at("compin"), at("threshold"), at("deessfreq"), at("hffreq")), (80.0, 1.0, -8.0, 6000.0, 2.0));
        // Every value is one the parameter can hold.
        for i in 0..p.preset_count() {
            for (v, info) in p.preset_values(i).unwrap().iter().zip(PARAMS.iter()) {
                assert_eq!(*v, info.clamp(*v), "{} in {}", info.id, p.preset_name(i));
            }
        }
    }

    #[test]
    fn every_pin_is_usable() {
        let plugin = BlueChannel::default();
        let errors = openplugin::core::identity::check_pins(&plugin);
        assert!(errors.is_empty(), "{errors:?}");
        assert!(plugin.vst3_ids().is_some_and(|ids| ids.is_valid()), "the VST3 class ids do not parse");
        assert!(plugin.aax_ids().is_some_and(|ids| ids.is_valid()));
    }

    #[test]
    fn a_refused_state_changes_nothing() {
        let mut a = BlueChannel::default();
        a.set_param("mix", 40.0);
        let before = a.values().to_vec();
        assert!(!a.load_state(&[]));
        assert!(!a.load_state(b"not a state"));
        assert_eq!(a.values(), &before[..]);
    }
}
