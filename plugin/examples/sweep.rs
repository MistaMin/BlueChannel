//! A host's parameter sweep, offline: each parameter to its ends and back, checking that the
//! output stays finite and audible. `cargo run --release --example sweep [offline]`
use blue_channel::{BlueChannel, PARAMS};
use openplugin::prelude::*;

fn render(p: &mut BlueChannel, seconds: f32, n: &mut u64) -> (f32, bool) {
    let mut peak = 0f32;
    let mut finite = true;
    for _ in 0..(seconds * 48_000.0 / 512.0) as usize {
        let ch: Vec<f32> = (0..512)
            .map(|i| {
                let t = (*n + i as u64) as f32 / 48_000.0;
                0.25 * (std::f32::consts::TAU * (100.0 + 3000.0 * (t * 0.7).fract()) * t).sin()
            })
            .collect();
        *n += 512;
        let mut io = vec![ch.clone(), ch];
        p.process(&mut ChannelsMut::new(&mut io), 512);
        for s in io.iter().flatten() {
            finite &= s.is_finite();
            peak = peak.max(s.abs());
        }
    }
    (peak, finite)
}

fn main() {
    let offline = std::env::args().nth(1).as_deref() == Some("offline");
    let mut p = BlueChannel::default();
    p.prepare(48_000.0, 512, 2);
    p.set_offline(offline);
    p.reset();
    let mut n = 0;
    println!("start: {:?}", render(&mut p, 1.0, &mut n));
    for info in PARAMS.iter() {
        let mut worst = (f32::MAX, true);
        for value in [info.max, info.min, info.default] {
            p.set_param(info.id, value);
            let (peak, finite) = render(&mut p, 0.5, &mut n);
            if value == info.default {
                worst = (peak, finite);
            }
        }
        if !worst.1 || worst.0 < 1e-4 {
            println!("after {:<12} back at its default: peak {:.6}, finite {}", info.id, worst.0, worst.1);
        }
    }
    println!("end: {:?}", render(&mut p, 1.0, &mut n));
}
