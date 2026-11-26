//! The states `openplugin-port capture` saved from the 1.0.0 JUCE release
//! (tests/fixtures) load into the port with the values the capture read back.

use blue_channel::{BlueChannel, PARAMS};
use openplugin::core::foreign_state::ForeignStateKind;
use openplugin::prelude::*;

const DEFAULT: &[u8] = include_bytes!("fixtures/000-default.clap-state.bin");
const SET: &[u8] = include_bytes!("fixtures/001-set.clap-state.bin");

fn loaded(blob: &[u8]) -> BlueChannel {
    let mut plugin = BlueChannel::default();
    // Start away from the defaults, so a value the load leaves alone shows.
    for p in PARAMS.iter() {
        plugin.set_param(p.id, p.max);
    }
    assert!(plugin.load_state(blob));
    plugin
}

#[test]
fn the_default_state_of_the_release_is_the_default_of_the_port() {
    let plugin = loaded(DEFAULT);
    for p in PARAMS.iter() {
        let value = plugin.param(p.id).unwrap();
        assert!((value - p.default).abs() <= 1e-4 * (1.0 + p.default.abs()), "{}: {value} for {}", p.id, p.default);
    }
}

#[test]
fn a_changed_state_of_the_release_lands_on_the_same_parameters() {
    // Captured with: Comp In = 1, Mix = 0.25 and Input = 1 as the release's 0..1 values.
    let plugin = loaded(SET);
    assert_eq!(plugin.param("compin"), Some(1.0));
    assert_eq!(plugin.param("mix"), Some(25.0));
    assert_eq!(plugin.param("source"), Some(2.0));
    let changed: Vec<_> = PARAMS.iter().filter(|p| (plugin.param(p.id).unwrap() - p.default).abs() > 1e-3).map(|p| p.id).collect();
    assert_eq!(changed, ["source", "compin", "mix"]);
}

#[test]
fn a_damaged_state_of_the_release_is_refused_and_changes_nothing() {
    let mut plugin = BlueChannel::default();
    plugin.set_param("mix", 40.0);
    let before = plugin.values().to_vec();
    for cut in [5, 8, 40, SET.len() / 2, SET.len() - 3] {
        assert!(!plugin.load_state(&SET[..cut]), "cut at {cut}");
    }
    let mut bad = SET.to_vec();
    let at = bad.windows(7).position(|w| w == b"value=\"").unwrap() + 7;
    bad[at] = b'x';
    assert!(!plugin.load_state(&bad));
    assert_eq!(plugin.values(), &before[..]);
}

/// The Vocal program as the release saved it through a VST3 host and through an
/// Audio Unit host, and its own state as an AAX host hands it over.
#[test]
fn the_release_states_of_vst3_audio_unit_and_aax_hosts_load() {
    let vocal = BlueChannel::default().preset_values(1).unwrap().to_vec();
    let aax = [b"juce".as_slice(), SET].concat();
    for (kind, blob, want) in [
        (ForeignStateKind::Vst3Component, include_bytes!("fixtures/002-vocal.vst3-component.bin").as_slice(), &vocal),
        (ForeignStateKind::AuClassInfo, include_bytes!("fixtures/002-vocal.au-classinfo.bin").as_slice(), &vocal),
        (ForeignStateKind::AaxChunk, aax.as_slice(), &loaded(SET).values().to_vec()),
    ] {
        let mut plugin = BlueChannel::default();
        for p in PARAMS.iter() {
            plugin.set_param(p.id, p.max);
        }
        assert!(plugin.load_foreign_state(kind, blob), "{kind:?}");
        for (p, (got, want)) in PARAMS.iter().zip(plugin.values().iter().zip(want.iter())) {
            assert!((got - want).abs() <= 1e-4 * (1.0 + want.abs()), "{kind:?} {}: {got} for {want}", p.id);
        }
    }
}

#[test]
fn a_foreign_state_that_is_not_the_release_s_is_refused_and_changes_nothing() {
    let mut plugin = BlueChannel::default();
    plugin.set_param("mix", 40.0);
    let before = plugin.values().to_vec();
    let own = plugin.save_state().unwrap();
    let vst3 = include_bytes!("fixtures/002-vocal.vst3-component.bin");
    for kind in [ForeignStateKind::Vst3Component, ForeignStateKind::Vst3Controller, ForeignStateKind::AuClassInfo, ForeignStateKind::AaxChunk] {
        for blob in [&b""[..], b"juce", b"elck1234", &own, &vst3[..40], SET.get(..SET.len() / 2).unwrap()] {
            assert!(!plugin.load_foreign_state(kind, blob), "{kind:?} took {} bytes", blob.len());
        }
    }
    assert_eq!(plugin.values(), &before[..]);
}
