//! Offline HQ renders at a few settings: `cargo run --release --features hq --example hq_probe [cases]`.
use blue_channel::BlueChannel;
use openplugin::prelude::*;

fn main() {
    let cases: usize = std::env::args().nth(1).and_then(|a| a.parse().ok()).unwrap_or(usize::MAX);
    for (channels, block, level, blocks, source) in [(1, 8, 0.02f32, 2, 0.0f32), (1, 8, 0.02, 2, 1.0), (2, 8, 0.02, 2, 0.0), (1, 64, 0.02, 8, 0.0), (1, 64, 0.1, 8, 0.0), (2, 64, 0.1, 8, 0.0), (2, 64, 0.1, 8, 1.0), (2, 512, 0.25, 4, 0.0)].into_iter().take(cases) {
        let mut p = BlueChannel::default();
        p.set_param("hq", 1.0);
        p.set_param("source", source);
        p.prepare(48_000.0, block, channels);
        p.set_offline(true);
        p.reset();
        let start = std::time::Instant::now();
        let mut peak = 0f32;
        let mut status = (blue_channel::HqStatus::Idle, String::new());
        for b in 0..blocks {
            let ch: Vec<f32> = (0..block).map(|i| level * (std::f32::consts::TAU * 1000.0 * (b * block + i + 1) as f32 / 48_000.0).sin()).collect();
            let mut io = vec![ch; channels];
            p.process(&mut ChannelsMut::new(&mut io), block);
            peak = io[0].iter().fold(peak, |m, s| m.max(s.abs()));
            status = p.hq_status();
            if status.0 != blue_channel::HqStatus::Solving {
                break;
            }
        }
        println!("ch {channels} block {block} level {level} source {source}: {:?} {} peak {peak:.4} in {:.1}s", status.0, status.1, start.elapsed().as_secs_f32());
    }
}
