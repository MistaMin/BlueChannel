//! What is on the panel: one list of widgets per page, in the page's own units.
//!
//! The analogue page is the front panel of the hardware the plugin models, 1560 x 450
//! units, with every control, lamp, meter and field where the 1.0 release has it
//! (Source/Plugin/PluginEditor.cpp; knob placement and caps from Designs/knobs.csv).
//! Nothing on it may move: legibility there comes from type, ink and crisp drawing.
//!
//! The Digital skin's two pages are laid out for reading, 1250 x 900 units, with no
//! text under 12 units.

use openplugin::dri::goodlookindri::design::{self, Item};

use crate::params::index as ix;

/// (x, y, w, h) in panel units.
pub type R = (f32, f32, f32, f32);

/// The Digital overview's two graphs.
pub const EQ_GRAPH: R = (324.0, 124.0, 902.0, 270.0);
pub const COMP_GRAPH: R = (24.0, 530.0, 320.0, 326.0);

/// The VU meter of the analogue panel.
pub const VU: R = (22.0, 22.0, 220.0, 128.0);

/// The output and input level bars of the analogue panel: the rectangles the release gave
/// them. The LEDs are a column 9 units wide at the right of each; the output bar prints the
/// dBFS scale to the left of its column.
pub const OUT_BAR: R = (1400.0, 30.0, 40.0, 150.0);
pub const IN_BAR: R = (1446.0, 30.0, 16.0, 150.0);

/// Text sizes of the Digital skin, in panel units: nothing on it is smaller than `SMALL`.
pub const SMALL: f32 = 12.0;
pub const TEXT: f32 = 13.0;

/// The height of a key with its legend on the analogue panel, and of a row of the Digital skin.
pub const KEY_H: f32 = 34.0;
pub const ROW_H: f32 = 28.0;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Page {
    Analogue,
    /// The Digital skin's graphs.
    Overview,
    /// The Digital skin's page with every control.
    Controls,
}

impl Page {
    /// The page's size in its own units.
    pub fn size(self) -> (f32, f32) {
        match self {
            Page::Analogue => (1560.0, 450.0),
            Page::Overview | Page::Controls => (1250.0, 900.0),
        }
    }
}

pub struct Theme {
    pub panel_top: u32,
    pub panel_bottom: u32,
    pub section: u32,
    pub section_edge: u32,
    pub ink_on_panel: u32,
    pub ink_on_section: u32,
    pub ink_dim: u32,
}

pub const ANALOGUE: Theme = Theme {
    panel_top: 0xff3c4fb0,
    panel_bottom: 0xff2d3c94,
    section: 0xffb9bdc4,
    section_edge: 0xff8a8f99,
    ink_on_panel: 0xffffffff,
    ink_on_section: 0xff1d2230,
    ink_dim: 0xff363c4a,
};

pub const LED_RED: u32 = 0xffff3b30;
pub const LED_GREEN: u32 = 0xff34d058;
pub const LED_AMBER: u32 = 0xffffc21a;
/// The Digital skin's palette.
pub const D_BACK: u32 = 0xff08111c;
pub const D_CARD: u32 = 0xff0e1a2a;
pub const D_LINE: u32 = 0xff3a5679;
pub const D_INK: u32 = 0xffdbe8ff;
pub const D_DIM: u32 = 0xff8fa5c4;
pub const D_ACCENT: u32 = 0xff39d0ff;
pub const D_KEY: u32 = 0xffdca757;

/// An editor setting that is not a parameter.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Prop {
    VuSource,
    VuCal,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LampSrc {
    Overload,
    KeyInput,
    DeEss,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Action {
    Skin(u8),
    Page(Page),
}

pub enum Kind {
    /// Rotary control: caption above, knob, end legends below.
    Knob { param: usize, item: Item, lo: &'static str, hi: &'static str, on_section: bool, enabled: bool },
    /// Square key with an LED cap on a switch parameter.
    Key { param: usize, label: &'static str, led: u32, on_section: bool },
    /// In / Comp / Gate keys on a four-state routing parameter.
    Route { param: usize },
    /// One key steps through the options, a column of lamps shows the current one.
    /// `hq` adds the HQ parameter as a last option (the Offline selector).
    Step { param: usize, label: &'static str, names: &'static [&'static str], led: u32, on_section: bool, key_w: f32, hq: bool },
    /// The same for an editor setting.
    PropStep { prop: Prop, label: &'static str, names: &'static [&'static str] },
    SkinKey,
    Lamp { label: &'static str, colour: u32, on_section: bool, src: LampSrc },
    Vu,
    /// LED bar in dBFS: the output (with the printed scale) or the input.
    Bar { output: bool },
    GateLeds,
    /// The HQ status line.
    Status { size: f32, colour: u32, centre: bool },

    // The Digital skin.
    Slider { param: usize, name: &'static str, dim_unless_filters: bool, enabled: bool },
    Toggle { param: usize, name: &'static str },
    /// A choice as a row of segments after its name. `hq` as in `Step`.
    Seg { param: usize, name: &'static str, names: &'static [&'static str], hq: bool },
    PropSeg { prop: Prop, names: &'static [&'static str] },
    Button { text: &'static str, action: Action },
    /// The large rotary of the overview, with its value below.
    Hero { param: usize, enabled: bool },
    Label { text: &'static str },
    /// Level readouts: `main` is the overview's four, else the Levels card's five.
    Readouts { main: bool },
    VuText,
    HqLamp,
    EqGraph,
    CompGraph,
}

pub struct Widget {
    pub rect: R,
    pub kind: Kind,
}

impl Widget {
    /// The parameters this widget controls.
    pub fn params(&self) -> Vec<usize> {
        match &self.kind {
            Kind::Knob { param, .. } | Kind::Key { param, .. } | Kind::Route { param } | Kind::Slider { param, .. } | Kind::Toggle { param, .. } | Kind::Hero { param, .. } => {
                vec![*param]
            }
            Kind::Step { param, hq, .. } | Kind::Seg { param, hq, .. } => {
                if *hq {
                    vec![*param, ix::HQ]
                } else {
                    vec![*param]
                }
            }
            Kind::EqGraph => vec![ix::HPF, ix::LPF, ix::LFGAIN, ix::LFFREQ, ix::HFGAIN, ix::HFFREQ, ix::LMFGAIN, ix::LMFFREQ, ix::LMFQ, ix::HMFGAIN, ix::HMFFREQ, ix::HMFQ],
            Kind::CompGraph => vec![ix::THRESHOLD, ix::RATIO, ix::MAKEUP],
            _ => Vec::new(),
        }
    }
}

pub const ROUTE_KEYS: [(&str, u32); 3] = [("In", LED_RED), ("Comp", LED_AMBER), ("Gate", LED_GREEN)];
const SOURCES: &[&str] = &["Mic", "Line", "Inst"];
const IMPEDANCES: &[&str] = &["Low", "Classic", "Med", "High"];
const COMP_TYPES: &[&str] = &["VCA", "Comp", "Lim"];
const DYN_POSITIONS: &[&str] = &["Post-EQ", "Pre-EQ"];
const FACTORS: &[&str] = &["1x", "2x", "4x", "8x", "16x"];
const OFFLINE: &[&str] = &["Same", "1x", "2x", "4x", "8x", "16x", "HQ"];
const ROUTES: &[&str] = &["Out", "In", "Comp", "Gate"];
const LOW_SHELVES: &[&str] = &["20/33", "56/95", "160/270", "460/655"];
const HIGH_SHELVES: &[&str] = &["1.5/2.2k", "3.3/4.7k", "6.8/10k", "15/18k"];
pub const VU_SOURCES: &[&str] = &["Input", "Comp GR", "Output"];
pub const VU_CALS: &[&str] = &["+4 dBu", "+18 dBu"];

/// The knob design (Designs/knobs.csv): one row per knob, keyed by parameter id.
pub const KNOB_DESIGN: &str = include_str!("../../../Designs/knobs.csv");

/// End legends of each knob, by parameter id.
const LEGENDS: &[(&str, &str, &str)] = &[
    ("micgain", "-8", "+60"),
    ("linegain", "-18", "+18"),
    ("trim", "0", "+20"),
    ("lpf", "307", "24k"),
    ("hpf", "20", "1.5k"),
    ("lmfgain", "-15", "+15"),
    ("lmfq", "0.85", "2.9"),
    ("hmfgain", "-15", "+15"),
    ("hmfq", "0.85", "2.9"),
    ("lfgain", "-18", "+18"),
    ("hfgain", "-18", "+18"),
    ("ratio", "1.5", "10"),
    ("attack", "0.1ms", "100ms"),
    ("release", "0.1s", "7s"),
    ("makeup", "0", "+20"),
    ("threshold", "-28", "+12"),
    ("mix", "dry", "comp"),
    ("range", "0", "80"),
    ("gatethresh", "-40", "+10"),
    ("hold", "20ms", "4s"),
    ("gaterelease", "0.1s", "5s"),
    ("deessfreq", "2.1k", "8.7k"),
    ("deessthresh", "-30", "+10"),
    ("output", "-60", "+6"),
];

fn param_of(id: &str) -> usize {
    crate::PARAMS.iter().position(|p| p.id == id).unwrap_or_else(|| panic!("no parameter {id}"))
}

/// The analogue panel's grey control fields.
pub const SECTIONS: [(R, &str); 8] = [
    ((252.0, 12.0, 300.0, 210.0), "Input"),
    ((560.0, 12.0, 150.0, 210.0), "Filters"),
    ((718.0, 12.0, 388.0, 210.0), "Parametric EQ"),
    ((1114.0, 12.0, 276.0, 210.0), "Shelving EQ"),
    ((252.0, 230.0, 420.0, 208.0), "Compressor"),
    ((680.0, 230.0, 340.0, 208.0), "Expander / Gate"),
    ((1028.0, 230.0, 180.0, 208.0), "De-Esser"),
    ((1216.0, 230.0, 170.0, 208.0), "Output"),
];

/// The Digital overview's cards.
pub const OVERVIEW_CARDS: [(R, &str); 4] = [
    ((12.0, 50.0, 290.0, 430.0), "INPUT"),
    ((312.0, 50.0, 926.0, 430.0), "EQUALIZER"),
    ((12.0, 490.0, 640.0, 378.0), "COMPRESSOR"),
    ((662.0, 490.0, 576.0, 378.0), "OUTPUT"),
];

/// The Digital "all controls" page's cards, three rows of 266 units.
pub const CARDS: [(R, &str); 9] = [
    ((12.0, 50.0, 300.0, 266.0), "INPUT"),
    ((322.0, 50.0, 280.0, 266.0), "FILTERS"),
    ((612.0, 50.0, 626.0, 266.0), "PARAMETRIC EQ"),
    ((12.0, 326.0, 626.0, 266.0), "SHELVING EQ"),
    ((648.0, 326.0, 590.0, 266.0), "COMPRESSOR"),
    ((12.0, 602.0, 300.0, 266.0), "EXPANDER / GATE"),
    ((322.0, 602.0, 290.0, 266.0), "DE-ESSER / ROUTING"),
    ((622.0, 602.0, 370.0, 266.0), "PROCESSING"),
    ((1002.0, 602.0, 236.0, 266.0), "LEVELS / OUTPUT"),
];

/// The widgets of a page. `source` is the input selector's position: the Mic
/// gain control shows for Mic, the Line one otherwise (disabled for Inst).
pub fn widgets(page: Page, source: usize) -> Vec<Widget> {
    match page {
        Page::Analogue => analogue(source),
        Page::Overview => overview(source),
        Page::Controls => controls(source),
    }
}

/// The analogue panel, rectangle for rectangle as in the release.
fn analogue(source: usize) -> Vec<Widget> {
    let mut w = Vec::new();
    let mut add = |rect: R, kind: Kind| w.push(Widget { rect, kind });
    let key = |param: usize, label: &'static str, led: u32, on_section: bool| Kind::Key { param, label, led, on_section };

    add(VU, Kind::Vu);
    add((24.0, 160.0, 100.0, 40.0), Kind::PropStep { prop: Prop::VuSource, label: "Meter", names: VU_SOURCES });
    add((136.0, 160.0, 104.0, 30.0), Kind::PropStep { prop: Prop::VuCal, label: "0 VU", names: VU_CALS });
    add((24.0, 288.0, 220.0, 24.0), Kind::Status { size: 11.5, colour: ANALOGUE.ink_on_panel, centre: true });

    // Knobs, from the design file. Mic and Line gain share a place.
    for item in design::read_design(KNOB_DESIGN).unwrap_or_default() {
        let param = param_of(&item.parameter);
        if (param == ix::MICGAIN && source != 0) || (param == ix::LINEGAIN && source == 0) {
            continue;
        }
        let (lo, hi) = LEGENDS.iter().find(|l| l.0 == item.parameter).map_or(("", ""), |l| (l.1, l.2));
        let rect = (item.x, item.y, item.width, item.height);
        add(rect, Kind::Knob { param, item, lo, hi, on_section: true, enabled: !(param == ix::LINEGAIN && source == 2) });
    }

    add((442.0, 24.0, 104.0, 44.0), Kind::Step { param: ix::SOURCE, label: "Select", names: SOURCES, led: LED_RED, on_section: true, key_w: 46.0, hq: false });
    add((442.0, 72.0, 104.0, 58.0), Kind::Step { param: ix::IMPEDANCE, label: "Imped.", names: IMPEDANCES, led: LED_RED, on_section: true, key_w: 46.0, hq: false });
    add((440.0, 150.0, 52.0, 34.0), key(ix::AIR, "Mic Air", LED_RED, true));
    add((494.0, 150.0, 52.0, 34.0), key(ix::PHASE, "Phase", LED_RED, true));
    add((266.0, 166.0, 90.0, 14.0), Kind::Lamp { label: "Audio O/L", colour: LED_RED, on_section: true, src: LampSrc::Overload });

    add((566.0, 22.0, 40.0, 34.0), key(ix::ALLEQ, "All EQ", LED_RED, true));
    add((608.0, 22.0, 98.0, 34.0), Kind::Route { param: ix::FILTROUTE });
    add((726.0, 22.0, 98.0, 34.0), Kind::Route { param: ix::PARAMROUTE });
    add((803.0, 160.0, 52.0, 34.0), key(ix::LMFX3, "x3", LED_RED, true));
    add((993.0, 160.0, 52.0, 34.0), key(ix::HMFX3, "x3", LED_RED, true));
    add((1122.0, 22.0, 98.0, 34.0), Kind::Route { param: ix::SHELFROUTE });
    add((1199.0, 160.0, 52.0, 34.0), key(ix::LFHIGH, "Hi Range", LED_RED, true));
    add((1335.0, 160.0, 52.0, 34.0), key(ix::HFHIGH, "Hi Range", LED_RED, true));

    add(OUT_BAR, Kind::Bar { output: true });
    add(IN_BAR, Kind::Bar { output: false });
    add((1470.0, 30.0, 74.0, 34.0), key(ix::SOFTLIMIT, "Soft Limit", LED_RED, false));
    add((1470.0, 76.0, 74.0, 34.0), key(ix::LINK, "Stereo Link", LED_GREEN, false));
    add((1474.0, 122.0, 74.0, 14.0), Kind::Lamp { label: "Key input", colour: LED_GREEN, on_section: false, src: LampSrc::KeyInput });

    add((24.0, 326.0, 116.0, 34.0), Kind::Step { param: ix::DYNPOS, label: "Dynamics", names: DYN_POSITIONS, led: LED_RED, on_section: false, key_w: 46.0, hq: false });
    add((150.0, 326.0, 52.0, 34.0), key(ix::BYPASS, "Bypass", LED_RED, false));

    add((260.0, 240.0, 52.0, 34.0), key(ix::COMPIN, "Comp In", LED_RED, true));
    add((314.0, 235.0, 88.0, 44.0), Kind::Step { param: ix::COMPTYPE, label: "Vintage", names: COMP_TYPES, led: LED_AMBER, on_section: true, key_w: 46.0, hq: false });
    add((404.0, 240.0, 52.0, 34.0), key(ix::COMPLISTEN, "Listen", LED_RED, true));
    add((458.0, 240.0, 52.0, 34.0), key(ix::COMPKEY, "Ext Key", LED_GREEN, true));
    add((512.0, 240.0, 52.0, 34.0), key(ix::AUTOREL, "Auto Rel", LED_RED, true));
    add((566.0, 240.0, 52.0, 34.0), key(ix::BLEND, "Blend", LED_RED, true));

    add((688.0, 240.0, 50.0, 34.0), key(ix::GATEIN, "Gate In", LED_RED, true));
    add((740.0, 240.0, 50.0, 34.0), key(ix::EXPAND, "Expand", LED_RED, true));
    add((792.0, 240.0, 50.0, 34.0), key(ix::GATELISTEN, "Listen", LED_RED, true));
    add((844.0, 240.0, 50.0, 34.0), key(ix::GATEKEY, "Ext Key", LED_GREEN, true));
    add((896.0, 240.0, 50.0, 34.0), key(ix::FASTATT, "Fast Att", LED_RED, true));
    add((948.0, 240.0, 50.0, 34.0), key(ix::HYST, "Hyst", LED_RED, true));
    add((700.0, 282.0, 140.0, 22.0), Kind::GateLeds);

    add((1036.0, 240.0, 52.0, 34.0), key(ix::DEESSIN, "De-Ess In", LED_RED, true));
    add((1092.0, 240.0, 52.0, 34.0), key(ix::DEESSLISTEN, "Listen", LED_RED, true));
    add((1150.0, 250.0, 54.0, 14.0), Kind::Lamp { label: "Active", colour: LED_RED, on_section: true, src: LampSrc::DeEss });
    add((1328.0, 290.0, 52.0, 34.0), key(ix::MUTE, "Mute", LED_RED, true));

    add((1400.0, 236.0, 150.0, 74.0), Kind::Step { param: ix::OVERSAMPLING, label: "Oversample", names: FACTORS, led: LED_RED, on_section: false, key_w: 58.0, hq: false });
    add((1400.0, 316.0, 150.0, 98.0), Kind::Step { param: ix::OFFLINEOS, label: "Offline", names: OFFLINE, led: LED_AMBER, on_section: false, key_w: 58.0, hq: true });
    add((1400.0, 408.0, 58.0, 34.0), Kind::SkinKey);
    w
}

/// What both Digital pages show along the top and the bottom.
fn header(w: &mut Vec<Widget>) {
    let mut add = |rect: R, kind: Kind| w.push(Widget { rect, kind });
    add((200.0, 8.0, 110.0, 30.0), Kind::Button { text: "OVERVIEW", action: Action::Page(Page::Overview) });
    add((316.0, 8.0, 140.0, 30.0), Kind::Button { text: "ALL CONTROLS", action: Action::Page(Page::Controls) });
    add((478.0, 8.0, 42.0, 30.0), Kind::Label { text: "Meter" });
    add((522.0, 9.0, 150.0, ROW_H), Kind::PropSeg { prop: Prop::VuSource, names: &["In", "GR", "Out"] });
    add((688.0, 8.0, 38.0, 30.0), Kind::Label { text: "0 VU" });
    add((728.0, 9.0, 170.0, ROW_H), Kind::PropSeg { prop: Prop::VuCal, names: &["+4 dBu", "+18 dBu"] });
    add((912.0, 8.0, 86.0, 30.0), Kind::VuText);
    add((1010.0, 8.0, 60.0, 30.0), Kind::HqLamp);
    add((1122.0, 8.0, 116.0, 30.0), Kind::Button { text: "ANALOGUE", action: Action::Skin(0) });
    add((14.0, 874.0, 1222.0, 20.0), Kind::Status { size: SMALL, colour: 0xffe2c887, centre: false });
}

fn slider(param: usize, name: &'static str, x: f32, y: f32, width: f32) -> (R, Kind) {
    ((x, y, width, ROW_H), Kind::Slider { param, name, dim_unless_filters: false, enabled: true })
}

fn toggle(param: usize, name: &'static str, x: f32, y: f32, width: f32) -> (R, Kind) {
    ((x, y, width, ROW_H), Kind::Toggle { param, name })
}

fn seg(param: usize, name: &'static str, names: &'static [&'static str], x: f32, y: f32, width: f32) -> (R, Kind) {
    ((x, y, width, ROW_H), Kind::Seg { param, name, names, hq: false })
}

fn overview(source: usize) -> Vec<Widget> {
    let mut w = Vec::new();
    header(&mut w);
    let mut put = |(rect, kind): (R, Kind)| w.push(Widget { rect, kind });
    let dimmed = |param: usize, name: &'static str, x: f32, y: f32, width: f32| ((x, y, width, ROW_H), Kind::Slider { param, name, dim_unless_filters: true, enabled: true });

    // Input.
    put(seg(ix::SOURCE, "Source", SOURCES, 24.0, 90.0, 266.0));
    put(((82.0, 130.0, 150.0, 176.0), Kind::Hero { param: if source == 0 { ix::MICGAIN } else { ix::LINEGAIN }, enabled: source != 2 }));
    put(slider(ix::TRIM, "Trim", 24.0, 316.0, 266.0));
    put(seg(ix::IMPEDANCE, "Impedance", IMPEDANCES, 24.0, 354.0, 266.0));
    put(toggle(ix::AIR, "AIR", 24.0, 396.0, 128.0));
    put(toggle(ix::PHASE, "PHASE", 162.0, 396.0, 128.0));
    put(((24.0, 438.0, 266.0, 30.0), Kind::Button { text: "GATE / DE-ESSER", action: Action::Page(Page::Controls) }));

    // Equaliser.
    put(seg(ix::FILTROUTE, "Filters", ROUTES, 324.0, 88.0, 290.0));
    put(seg(ix::PARAMROUTE, "Mids", ROUTES, 630.0, 88.0, 290.0));
    put(seg(ix::SHELFROUTE, "Shelves", ROUTES, 936.0, 88.0, 290.0));
    put((EQ_GRAPH, Kind::EqGraph));
    put(dimmed(ix::HPF, "High pass", 324.0, 402.0, 441.0));
    put(dimmed(ix::LPF, "Low pass", 785.0, 402.0, 441.0));
    for (i, (param, name)) in [(ix::ALLEQ, "EQ ACTIVE"), (ix::LFHIGH, "LF HI RANGE"), (ix::LMFX3, "LMF x3"), (ix::HMFX3, "HMF x3"), (ix::HFHIGH, "HF HI RANGE")].into_iter().enumerate() {
        put(toggle(param, name, 324.0 + 182.0 * i as f32, 440.0, 174.0));
    }

    // Compressor.
    put((COMP_GRAPH, Kind::CompGraph));
    put(seg(ix::COMPTYPE, "Type", COMP_TYPES, 360.0, 534.0, 280.0));
    put(slider(ix::ATTACK, "Attack", 360.0, 580.0, 280.0));
    put(slider(ix::RELEASE, "Release", 360.0, 618.0, 280.0));
    put(slider(ix::MAKEUP, "Makeup", 360.0, 656.0, 280.0));
    put(toggle(ix::COMPIN, "ACTIVE", 360.0, 706.0, 280.0));
    put(toggle(ix::AUTOREL, "AUTO REL", 360.0, 744.0, 136.0));
    put(toggle(ix::COMPKEY, "EXT KEY", 504.0, 744.0, 136.0));

    // Output.
    put(((684.0, 536.0, 190.0, 220.0), Kind::Hero { param: ix::OUTPUT, enabled: true }));
    put(((906.0, 540.0, 320.0, 148.0), Kind::Readouts { main: true }));
    put(toggle(ix::MUTE, "MUTE", 906.0, 716.0, 100.0));
    put(toggle(ix::SOFTLIMIT, "LIMIT", 1014.0, 716.0, 100.0));
    put(toggle(ix::LINK, "LINK", 1122.0, 716.0, 104.0));
    put(toggle(ix::BYPASS, "BYPASS", 906.0, 756.0, 320.0));
    w
}

fn controls(source: usize) -> Vec<Widget> {
    let mut w = Vec::new();
    header(&mut w);
    let mut put = |(rect, kind): (R, Kind)| w.push(Widget { rect, kind });
    // The rows of a card: 34 units apart under its title.
    let row = |card: usize, n: usize| CARDS[card].0 .1 + 40.0 + 34.0 * n as f32;
    // Column `n` of `count` equal columns in a card: (x, width).
    let cell = |card: usize, n: usize, count: usize, gap: f32| {
        let width = (CARDS[card].0 .2 - 24.0 - gap * (count - 1) as f32) / count as f32;
        (CARDS[card].0 .0 + 12.0 + (width + gap) * n as f32, width)
    };
    let whole = |card: usize| cell(card, 0, 1, 0.0);
    // The two columns of a wide card.
    let half = |card: usize, n: usize| cell(card, n, 2, 24.0);

    // Input.
    let (x, width) = whole(0);
    put(seg(ix::SOURCE, "Source", SOURCES, x, row(0, 0), width));
    put(seg(ix::IMPEDANCE, "Impedance", IMPEDANCES, x, row(0, 1), width));
    let gain = if source == 0 { (ix::MICGAIN, "Mic gain") } else { (ix::LINEGAIN, "Line gain") };
    put(((x, row(0, 2), width, ROW_H), Kind::Slider { param: gain.0, name: gain.1, dim_unless_filters: false, enabled: source != 2 }));
    put(slider(ix::TRIM, "Trim", x, row(0, 3), width));
    for (n, (param, name)) in [(ix::AIR, "AIR"), (ix::PHASE, "PHASE")].into_iter().enumerate() {
        let (x, width) = cell(0, n, 2, 8.0);
        put(toggle(param, name, x, row(0, 4), width));
    }

    // Filters.
    let (x, width) = whole(1);
    put(seg(ix::FILTROUTE, "Route", ROUTES, x, row(1, 0), width));
    put(slider(ix::HPF, "High pass", x, row(1, 1), width));
    put(slider(ix::LPF, "Low pass", x, row(1, 2), width));
    put(toggle(ix::ALLEQ, "ALL EQ", x, row(1, 4), width));

    // Parametric equaliser: the low mid band on the left, the high mid band on the right.
    let (x, width) = half(2, 0);
    put(seg(ix::PARAMROUTE, "Route", ROUTES, x, row(2, 0), width));
    for (n, band) in [[(ix::LMFGAIN, "LMF gain"), (ix::LMFFREQ, "LMF freq"), (ix::LMFQ, "LMF Q"), (ix::LMFX3, "LMF x3")], [(ix::HMFGAIN, "HMF gain"), (ix::HMFFREQ, "HMF freq"), (ix::HMFQ, "HMF Q"), (ix::HMFX3, "HMF x3")]].into_iter().enumerate() {
        let (x, width) = half(2, n);
        for (i, (param, name)) in band.into_iter().enumerate() {
            put(if i == 3 { toggle(param, name, x, row(2, 4), width) } else { slider(param, name, x, row(2, i + 1), width) });
        }
    }

    // Shelving equaliser: low on the left, high on the right.
    let (x, width) = half(3, 0);
    put(seg(ix::SHELFROUTE, "Route", ROUTES, x, row(3, 0), width));
    for (n, (gain, gain_name, freq, freq_name, names, range, range_name)) in
        [(ix::LFGAIN, "LF gain", ix::LFFREQ, "LF freq", LOW_SHELVES, ix::LFHIGH, "LF HI RANGE"), (ix::HFGAIN, "HF gain", ix::HFFREQ, "HF freq", HIGH_SHELVES, ix::HFHIGH, "HF HI RANGE")].into_iter().enumerate()
    {
        let (x, width) = half(3, n);
        put(slider(gain, gain_name, x, row(3, 1), width));
        put(seg(freq, freq_name, names, x, row(3, 2), width));
        put(toggle(range, range_name, x, row(3, 3), width));
    }

    // Compressor.
    let (x, width) = half(4, 0);
    put(seg(ix::COMPTYPE, "Type", COMP_TYPES, x, row(4, 0), width));
    for (n, (param, name)) in [(ix::RATIO, "Ratio"), (ix::THRESHOLD, "Threshold"), (ix::MAKEUP, "Makeup"), (ix::ATTACK, "Attack"), (ix::RELEASE, "Release"), (ix::MIX, "Mix")].into_iter().enumerate() {
        let (x, width) = half(4, n / 3);
        put(slider(param, name, x, row(4, 1 + n % 3), width));
    }
    for (n, (param, name)) in [(ix::COMPIN, "ACTIVE"), (ix::AUTOREL, "AUTO REL"), (ix::COMPKEY, "EXT KEY"), (ix::COMPLISTEN, "LISTEN"), (ix::BLEND, "BLEND")].into_iter().enumerate() {
        let (x, width) = cell(4, n, 5, 8.0);
        put(toggle(param, name, x, row(4, 5), width));
    }

    // Expander / gate.
    let (x, width) = whole(5);
    for (n, (param, name)) in [(ix::GATETHRESH, "Threshold"), (ix::RANGE, "Range"), (ix::HOLD, "Hold"), (ix::GATERELEASE, "Release")].into_iter().enumerate() {
        put(slider(param, name, x, row(5, n), width));
    }
    for (n, (param, name)) in [(ix::GATEIN, "ACTIVE"), (ix::EXPAND, "EXPAND"), (ix::GATEKEY, "EXT KEY"), (ix::GATELISTEN, "LISTEN"), (ix::FASTATT, "FAST ATT"), (ix::HYST, "HYST")].into_iter().enumerate() {
        let (x, width) = cell(5, n % 3, 3, 8.0);
        put(toggle(param, name, x, row(5, 4 + n / 3), width));
    }

    // De-esser and where the dynamics sit.
    let (x, width) = whole(6);
    put(slider(ix::DEESSFREQ, "Frequency", x, row(6, 0), width));
    put(slider(ix::DEESSTHRESH, "Threshold", x, row(6, 1), width));
    for (n, (param, name)) in [(ix::DEESSIN, "ACTIVE"), (ix::DEESSLISTEN, "LISTEN")].into_iter().enumerate() {
        let (x, width) = cell(6, n, 2, 8.0);
        put(toggle(param, name, x, row(6, 2), width));
    }
    put(seg(ix::DYNPOS, "Dynamics", DYN_POSITIONS, x, row(6, 4), width));

    // Processing.
    let (x, width) = whole(7);
    put(seg(ix::OVERSAMPLING, "Playback", FACTORS, x, row(7, 0), width));
    put(((x, row(7, 1), width, ROW_H), Kind::Seg { param: ix::OFFLINEOS, name: "Offline", names: OFFLINE, hq: true }));
    put(((x, row(7, 2), width, ROW_H), Kind::Label { text: "Circuit model at every factor" }));
    put(toggle(ix::BYPASS, "BYPASS", x, row(7, 4), width));

    // Levels and output.
    let (x, width) = whole(8);
    let top = CARDS[8].0 .1;
    put(((x, top + 38.0, width, 144.0), Kind::Readouts { main: false }));
    put(slider(ix::OUTPUT, "Output", x, top + 188.0, width));
    for (n, (param, name)) in [(ix::MUTE, "MUTE"), (ix::SOFTLIMIT, "LIMIT"), (ix::LINK, "LINK")].into_iter().enumerate() {
        let (x, width) = cell(8, n, 3, 6.0);
        put(toggle(param, name, x, top + 226.0, width));
    }
    w
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::PARAMS;

    fn covered(page: Page, source: usize) -> Vec<bool> {
        let mut seen = vec![false; PARAMS.len()];
        for widget in widgets(page, source) {
            for p in widget.params() {
                seen[p] = true;
            }
        }
        seen
    }

    /// Every parameter has a control on the analogue panel and on the Digital
    /// skin's "all controls" page, whichever input is selected. Mic gain shows
    /// for Mic and Line gain for the others, as in the release.
    #[test]
    fn every_parameter_has_a_control_on_both_skins() {
        for page in [Page::Analogue, Page::Controls] {
            let by_source: Vec<_> = (0..3).map(|s| covered(page, s)).collect();
            for (i, p) in PARAMS.iter().enumerate() {
                let any = by_source.iter().any(|c| c[i]);
                assert!(any, "{} has no control on {page:?}", p.id);
                if i != ix::MICGAIN && i != ix::LINEGAIN {
                    assert!(by_source.iter().all(|c| c[i]), "{} is missing on {page:?} for some input", p.id);
                }
            }
            assert!(by_source[0][ix::MICGAIN] && !by_source[0][ix::LINEGAIN] && by_source[1][ix::LINEGAIN] && by_source[2][ix::LINEGAIN]);
        }
    }

    #[test]
    fn every_widget_is_inside_the_panel() {
        for page in [Page::Analogue, Page::Overview, Page::Controls] {
            let (panel_w, panel_h) = page.size();
            for (n, w) in widgets(page, 0).iter().enumerate() {
                let (x, y, width, height) = w.rect;
                assert!(x >= 0.0 && y >= 0.0 && x + width <= panel_w && y + height <= panel_h && width > 0.0 && height > 0.0, "{page:?} widget {n}: {:?}", w.rect);
            }
        }
    }

    #[test]
    fn every_row_of_the_knob_design_names_a_parameter() {
        let items = design::read_design(KNOB_DESIGN).unwrap();
        assert_eq!(items.len(), 28);
        for item in &items {
            param_of(&item.parameter);
        }
    }
}
