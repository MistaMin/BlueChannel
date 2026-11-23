//! HQ Mode: the offline component solver. With the `hq` feature the solver is
//! in the build (`cargo test --features hq --test hq`, which needs a shared
//! ngspice library); without it the build behaves as the release's builds
//! without the solver do.

use blue_channel::{BlueChannel, HqStatus};
use openplugin::prelude::*;

const RATE: f32 = 48_000.0;
const BLOCK: usize = 64;

fn sine(block: usize, channels: usize) -> Vec<Vec<f32>> {
    let channel: Vec<f32> = (0..BLOCK).map(|i| 0.1 * (std::f32::consts::TAU * 1000.0 * (block * BLOCK + i) as f32 / RATE).sin()).collect();
    vec![channel; channels]
}

/// Render `blocks` blocks and return the left channel.
fn render(plugin: &mut Fixture, blocks: usize) -> Vec<f32> {
    let mut out = Vec::new();
    for block in 0..blocks {
        let mut io = sine(block, plugin.channels);
        plugin.plugin.process(&mut ChannelsMut::new(&mut io), BLOCK);
        out.extend_from_slice(&io[0]);
    }
    out
}

fn rms(x: &[f32]) -> f32 {
    (x.iter().map(|s| s * s).sum::<f32>() / x.len() as f32).sqrt()
}

struct Fixture {
    plugin: BlueChannel,
    channels: usize,
}

impl Fixture {
    fn hq_status(&mut self) -> (HqStatus, String) {
        self.plugin.hq_status()
    }
}

fn plugin(hq: bool, offline: bool, channels: usize) -> Fixture {
    let mut p = BlueChannel::default();
    p.set_param("hq", f32::from(u8::from(hq)));
    p.prepare(RATE, BLOCK, channels);
    p.set_offline(offline);
    p.reset();
    Fixture { plugin: p, channels }
}

#[test]
fn hq_arms_in_real_time_and_plays_the_standard_engine() {
    let mut standard = plugin(false, false, 2);
    let mut armed = plugin(true, false, 2);
    let (a, b) = (render(&mut standard, 8), render(&mut armed, 8));
    assert_eq!(a, b, "HQ changes nothing until the host renders offline");
    assert_eq!(standard.hq_status().0, HqStatus::Idle);
    assert_eq!(armed.hq_status().0, if BlueChannel::hq_compiled() { HqStatus::Armed } else { HqStatus::Unavailable });
}

#[cfg(not(feature = "hq"))]
#[test]
fn without_the_solver_an_offline_hq_render_is_muted_and_says_so() {
    assert!(!BlueChannel::hq_compiled());
    let mut p = plugin(true, true, 2);
    let out = render(&mut p, 4);
    assert!(out.iter().all(|s| *s == 0.0));
    assert_eq!(p.hq_status().0, HqStatus::Unavailable);
    assert_eq!(HqStatus::Unavailable.text(), "Offline HQ unavailable");
    // Turning HQ off again renders normally.
    p.plugin.set_param("hq", 0.0);
    assert!(rms(&render(&mut p, 4)) > 0.01);
}

#[cfg(feature = "hq")]
#[test]
fn an_offline_hq_render_solves_the_circuit_and_agrees_with_the_standard_engine() {
    assert!(BlueChannel::hq_compiled());
    let blocks = 6; // 8 ms: the solver takes about a second per millisecond
    let mut standard = plugin(false, true, 1);
    let mut hq = plugin(true, true, 1);
    let reference = render(&mut standard, blocks);
    let solved = render(&mut hq, blocks);
    let (status, error) = hq.hq_status();
    assert_eq!(status, HqStatus::Solving, "{error}");
    assert!(solved.iter().all(|s| s.is_finite()));
    // The same circuit by two methods: the level at 1 kHz agrees within a decibel
    // once both have settled (docs/HQ_MODE.md gives the measured differences).
    let settled = blocks * BLOCK / 2;
    let (a, b) = (rms(&reference[settled..]), rms(&solved[settled..]));
    let difference = 20.0 * (b / a).log10();
    println!("standard {a:.5}, HQ {b:.5}: {difference:+.2} dB");
    assert!(a > 0.01 && difference.abs() < 1.0, "standard {a}, HQ {b}");
}

/// A solve that fails mutes the render and says so; Standard audio is never
/// substituted (docs/HQ_MODE.md). Today a two-channel solve is such a failure:
/// the solver does not find the operating point of the stereo circuit ("timestep
/// too small"), in the JUCE build's renderer as well. When that is fixed this
/// test needs another way to make a solve fail.
#[cfg(feature = "hq")]
#[test]
fn a_failed_solve_mutes_the_render_and_reports_it() {
    let mut hq = plugin(true, true, 2);
    let out = render(&mut hq, 2);
    let (status, error) = hq.hq_status();
    assert_eq!(status, HqStatus::Failed, "a stereo solve now works: see the comment");
    assert!(!error.is_empty());
    assert!(out.iter().all(|s| *s == 0.0));
}
