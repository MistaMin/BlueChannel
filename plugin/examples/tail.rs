#![allow(clippy::needless_range_loop)]
use blue_channel::BlueChannel;
use openplugin::prelude::*;
fn main() {
    let sets: &[&[(&str, f32)]] = &[
        &[],
        &[("source", 1.0)],
        &[("source", 2.0)],
        &[("filtroute", 1.0)],
        &[("filtroute", 1.0), ("hpf", 1500.0), ("lpf", 310.0)],
        &[("shelfroute", 1.0), ("lfgain", 17.5), ("lffreq", 0.0), ("paramroute", 1.0), ("lmfgain", 14.8), ("lmffreq", 36.0), ("lmfq", 2.9)],
        &[("compin", 1.0), ("threshold", -28.0), ("release", 7.0), ("makeup", 20.0)],
        &[("gatein", 1.0), ("gatethresh", -40.0), ("hold", 4.0), ("gaterelease", 5.0)],
        &[("micgain", 11.0), ("trim", 20.0)],
        &[("air", 1.0), ("impedance", 3.0), ("oversampling", 0.0)],
    ];
    for rate in [44_100.0f32, 96_000.0] {
        for set in sets {
            let mut p = BlueChannel::default();
            for (id, v) in *set { assert!(p.set_param(id, *v)); }
            p.prepare(rate, 512, 2);
            p.reset();
            let mut seed = 1u32;
            let mut last = 0usize; let mut n = 0usize; let mut peak = 0f32;
            let total = (rate * 12.0) as usize; let stop = rate as usize;
            while n < total {
                let mut io = vec![vec![0.0f32; 512]; 2];
                for i in 0..512 { if n + i < stop { seed = seed.wrapping_mul(1664525).wrapping_add(1013904223); let s = ((seed >> 8) as f32 / 8388608.0 - 1.0) * 0.25; io[0][i] = s; io[1][i] = s; } }
                p.process(&mut ChannelsMut::new(&mut io), 512);
                for i in 0..512 { let a = io[0][i].abs().max(io[1][i].abs()); if n + i < stop { peak = peak.max(a); } else if a > 3.16e-5 { last = n + i - stop; } }
                n += 512;
            }
            println!("{rate} {:?}: peak {:.3}, above -90 dBFS until {:.3} s", set, peak, last as f32 / rate);
        }
    }
}
