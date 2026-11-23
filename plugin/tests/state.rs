//! The states `openplugin-port capture` saved from the 1.0.0 JUCE release
//! (tests/fixtures) load into the port with the values the capture read back.

use blue_channel::{BlueChannel, PARAMS};
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
