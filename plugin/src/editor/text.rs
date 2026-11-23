//! Value text, as the 1.0 release printed it (Source/Plugin/Params.cpp).

use openplugin::prelude::*;

use crate::params::index as ix;
use crate::PARAMS;

pub fn hz(hz: f32) -> String {
    if hz >= 10_000.0 {
        format!("{:.1} kHz", hz / 1000.0)
    } else if hz >= 1000.0 {
        format!("{:.2} kHz", hz / 1000.0)
    } else if hz >= 100.0 {
        format!("{} Hz", hz.round())
    } else {
        format!("{hz:.1} Hz")
    }
}

fn db(v: f32, decimals: usize) -> String {
    // No "-0.0": a value that rounds to zero prints as zero.
    let scale = 10f32.powi(decimals as i32);
    let v = (v * scale).round() / scale;
    let v = if v == 0.0 { 0.0 } else { v };
    format!("{}{v:.decimals$} dB", if v > 0.0 { "+" } else { "" })
}

/// What a control shows for a parameter's plain value.
pub fn value(param: usize, v: f32) -> String {
    let info = &PARAMS[param];
    match info.unit {
        Unit::Toggle => (if v > 0.5 { "On" } else { "Off" }).to_string(),
        Unit::Choice => info.choices.get(v.round().max(0.0) as usize).copied().unwrap_or("").to_string(),
        Unit::Hz => hz(v),
        Unit::Db if param == ix::RANGE => db(v, 0),
        Unit::Db => db(v, 1),
        Unit::Ratio => format!("{v:.1}:1"),
        Unit::Percent => format!("{} %", v.round()),
        Unit::Ms => {
            if v < 1.0 {
                format!("{v:.2} ms")
            } else if v < 100.0 {
                format!("{v:.1} ms")
            } else {
                format!("{v:.0} ms")
            }
        }
        Unit::Seconds if param == ix::HOLD && v < 1.0 => format!("{} ms", (v * 1000.0).round()),
        Unit::Seconds => format!("{v:.2} s"),
        _ => format!("{v:.2}"),
    }
}

pub fn low_shelf(position: usize, high: bool) -> &'static str {
    const LO: [&str; 4] = ["20 Hz", "56 Hz", "160 Hz", "460 Hz"];
    const HI: [&str; 4] = ["33 Hz", "95 Hz", "270 Hz", "655 Hz"];
    if high { HI[position.min(3)] } else { LO[position.min(3)] }
}

pub fn high_shelf(position: usize, high: bool) -> &'static str {
    const LO: [&str; 4] = ["1.5 kHz", "3.3 kHz", "6.8 kHz", "15 kHz"];
    const HI: [&str; 4] = ["2.2 kHz", "4.7 kHz", "10 kHz", "18 kHz"];
    if high { HI[position.min(3)] } else { LO[position.min(3)] }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn values_print_as_the_release_printed_them() {
        assert_eq!(hz(19.62), "19.6 Hz");
        assert_eq!(hz(209.4), "209 Hz");
        assert_eq!(hz(1506.96), "1.51 kHz");
        assert_eq!(hz(23611.45), "23.6 kHz");
        assert_eq!(value(ix::TRIM, 5.2), "+5.2 dB");
        assert_eq!(value(ix::THRESHOLD, -0.04), "0.0 dB");
        assert_eq!(value(ix::RANGE, 20.0), "+20 dB");
        assert_eq!(value(ix::RATIO, 3.0), "3.0:1");
        assert_eq!(value(ix::ATTACK, 0.5), "0.50 ms");
        assert_eq!(value(ix::ATTACK, 10.0), "10.0 ms");
        assert_eq!(value(ix::ATTACK, 100.0), "100 ms");
        assert_eq!(value(ix::HOLD, 0.1), "100 ms");
        assert_eq!(value(ix::HOLD, 2.0), "2.00 s");
        assert_eq!(value(ix::MIX, 60.2), "60 %");
        assert_eq!(value(ix::LMFQ, 1.2), "1.20");
        assert_eq!(value(ix::SOURCE, 2.0), "Inst");
    }
}
