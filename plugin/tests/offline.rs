//! The render policy of the release (docs/OFFLINE_RENDER.md): playback and
//! realtime export use the playback oversampling; an offline render uses the
//! Offline setting when it is not "Same as playback", on a bank of its own
//! that starts from the same state at every bounce.

use blue_channel::BlueChannel;
use openplugin::prelude::*;

fn tone(block: usize) -> Vec<Vec<f32>> {
    let ch: Vec<f32> = (0..256).map(|i| 0.5 * (std::f32::consts::TAU * 5000.0 * (block * 256 + i) as f32 / 48_000.0).sin()).collect();
    vec![ch.clone(), ch]
}

fn render(p: &mut BlueChannel, blocks: usize) -> Vec<f32> {
    (0..blocks)
        .flat_map(|b| {
            let mut io = tone(b);
            p.process(&mut ChannelsMut::new(&mut io), 256);
            io.swap_remove(0)
        })
        .collect()
}

fn plugin(playback: f32, offline_setting: f32) -> BlueChannel {
    let mut p = BlueChannel::default();
    p.set_param("oversampling", playback);
    p.set_param("offlineos", offline_setting);
    p.set_param("micgain", 6.0); // enough gain for the oversampling factor to matter
    p.prepare(48_000.0, 256, 2);
    p.reset();
    p
}

#[test]
fn an_offline_render_uses_the_offline_factor_on_its_own_bank() {
    // Playback at 1x, offline at 8x.
    let mut p = plugin(0.0, 4.0);
    let live = render(&mut p, 8);
    assert_eq!((p.telemetry().active_factor, p.telemetry().rendering), (1, 0));
    p.set_offline(true);
    let bounce = render(&mut p, 8);
    assert_eq!((p.telemetry().active_factor, p.telemetry().rendering), (8, 1));
    assert!(live != bounce);
    // A second bounce starts from the same state: the same samples.
    p.set_offline(false);
    render(&mut p, 3);
    p.set_offline(true);
    assert!(render(&mut p, 8) == bounce, "a second bounce differs from the first");
}

#[test]
fn same_as_playback_renders_offline_exactly_as_it_plays() {
    let mut a = plugin(1.0, 0.0);
    let mut b = plugin(1.0, 0.0);
    b.set_offline(true);
    assert!(render(&mut a, 8) == render(&mut b, 8));
    assert_eq!((b.telemetry().active_factor, b.telemetry().rendering), (2, 0));
}
