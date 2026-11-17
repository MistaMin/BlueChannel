//! The factory programs of the 1.0 release. A program sets the sound: the
//! oversampling, offline and HQ settings are left alone (they are flagged
//! `PRESET_EXCLUDE`), and every other parameter a program does not name is at
//! its default.

use std::sync::OnceLock;

use crate::{COUNT, PARAMS};

pub struct Program {
    pub name: &'static str,
    values: &'static [(&'static str, f32)],
}

pub static PROGRAMS: [Program; 6] = [
    Program { name: "Init", values: &[] },
    Program {
        name: "Vocal",
        values: &[
            ("filtroute", 1.0), ("hpf", 80.0), ("paramroute", 1.0), ("lmfgain", -2.5), ("lmffreq", 300.0), ("hmfgain", 2.0), ("hmffreq", 3000.0),
            ("shelfroute", 1.0), ("hfgain", 3.0), ("hffreq", 2.0), ("hfhigh", 1.0), ("compin", 1.0), ("ratio", 3.0), ("threshold", -8.0),
            ("attack", 5.0), ("release", 0.25), ("autorel", 1.0), ("makeup", 4.0), ("deessin", 1.0), ("deessthresh", -12.0), ("deessfreq", 6000.0),
        ],
    },
    Program {
        name: "Drum Bus",
        values: &[
            ("shelfroute", 1.0), ("lfgain", 3.0), ("lffreq", 1.0), ("hfgain", 2.5), ("hffreq", 2.0), ("compin", 1.0), ("ratio", 4.0),
            ("threshold", -6.0), ("attack", 20.0), ("release", 0.15), ("makeup", 3.0), ("blend", 1.0), ("mix", 60.0),
        ],
    },
    Program {
        name: "Bass DI",
        values: &[
            ("source", 2.0), ("trim", 6.0), ("filtroute", 1.0), ("hpf", 35.0), ("lpf", 6000.0), ("paramroute", 1.0), ("lmfgain", 3.0),
            ("lmffreq", 90.0), ("compin", 1.0), ("comptype", 1.0), ("ratio", 4.0), ("threshold", -10.0), ("makeup", 4.0),
        ],
    },
    Program {
        name: "Gated Toms",
        values: &[
            ("filtroute", 3.0), ("hpf", 80.0), ("lpf", 2000.0), ("gatein", 1.0), ("gatethresh", -20.0), ("range", 40.0), ("hold", 0.15),
            ("gaterelease", 0.3), ("fastatt", 1.0),
        ],
    },
    Program {
        name: "Mix Polish",
        values: &[
            ("shelfroute", 1.0), ("lfgain", 1.5), ("lffreq", 0.0), ("lfhigh", 1.0), ("hfgain", 1.5), ("hffreq", 3.0), ("hfhigh", 1.0),
            ("compin", 1.0), ("ratio", 1.8), ("threshold", -4.0), ("attack", 30.0), ("autorel", 1.0), ("makeup", 1.0), ("softlimit", 1.0),
        ],
    },
];

/// Every parameter's plain value in program `index`, in table order.
pub fn values(index: usize) -> Option<&'static [f32]> {
    static TABLES: OnceLock<Vec<[f32; COUNT]>> = OnceLock::new();
    let tables = TABLES.get_or_init(|| {
        PROGRAMS
            .iter()
            .map(|program| {
                let mut table: [f32; COUNT] = std::array::from_fn(|i| PARAMS[i].default);
                for (id, value) in program.values {
                    let at = PARAMS.iter().position(|p| p.id == *id).expect("a program names a parameter the table has");
                    table[at] = PARAMS[at].clamp(*value);
                }
                table
            })
            .collect()
    });
    tables.get(index).map(|t| &t[..])
}
