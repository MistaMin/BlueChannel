//! The parameter table: ids, order, ranges and choice lists of Blue Channel 1.0
//! (Source/Plugin/Params.cpp, docs/PARAMETERS.md). They are part of saved
//! sessions and automation: never change the meaning of an existing id, append.
//!
//! Each parameter is pinned to the id the JUCE build showed hosts, read by
//! `openplugin-port scan` from the released 1.0.0 CLAP, VST3 and Audio Unit:
//! CLAP has the 32-bit hash of the id string, VST3 and Audio Unit the same
//! hash without its top bit. AAX uses the id string itself, which is the
//! default. The ranges are the model's own constants (Source/Model), as `f32`.

use openplugin::prelude::*;

static ROUTES: &[&str] = &["Out", "In", "Comp key", "Gate key"];
// Input-to-output gain of each switch position, as the 1.0 release printed it.
static MIC_GAINS: &[&str] = &[
    "-7.83314 dB", "0 dB", "+4.5257 dB", "+10.9362 dB", "+16.9779 dB", "+22.5214 dB", "+29.4406 dB", "+35.419 dB", "+41.4141 dB", "+47.8061 dB",
    "+53.977 dB", "+59.9199 dB",
];
static LINE_GAINS: &[&str] = &["-17.8418 dB", "-12.0678 dB", "-5.94699 dB", "0 dB", "+5.94699 dB", "+12.0678 dB", "+17.8418 dB"];
static OVERSAMPLING: &[&str] = &["1x", "2x", "4x", "8x", "16x Derp mode"];
static OFFLINE_OVERSAMPLING: &[&str] = &["Same as playback", "1x", "2x", "4x", "8x", "16x Derp mode"];

pub const COUNT: usize = 62;

pub static PARAMS: [ParamInfo; COUNT] = [
    // Input
    ParamInfo::new("source", "Input", 0.0, 2.0, 0.0, Unit::None).with_choices(&["Mic", "Line", "Inst"])
        .with_clap_id(0xCA90681B).with_vst3_id(0x4A90681B).with_au_id(0x4A90681B),
    ParamInfo::new("micgain", "Mic Gain", 0.0, 11.0, 1.0, Unit::None).with_choices(MIC_GAINS)
        .with_clap_id(0x3ED77646).with_vst3_id(0x3ED77646).with_au_id(0x3ED77646),
    ParamInfo::new("linegain", "Line Gain", 0.0, 6.0, 3.0, Unit::None).with_choices(LINE_GAINS)
        .with_clap_id(0x46E1E173).with_vst3_id(0x46E1E173).with_au_id(0x46E1E173),
    ParamInfo::new("trim", "Trim", 0.0, 20.8097, 0.0, Unit::Db)
        .with_clap_id(0x00367422).with_vst3_id(0x00367422).with_au_id(0x00367422),
    ParamInfo::new("phase", "Phase", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x065B097B).with_vst3_id(0x065B097B).with_au_id(0x065B097B),
    ParamInfo::new("impedance", "Impedance", 0.0, 3.0, 1.0, Unit::None).with_choices(&["Low 600", "Classic 1.4k", "Medium 2.4k", "High 6.8k"])
        .with_clap_id(0xE1B3F53A).with_vst3_id(0x61B3F53A).with_au_id(0x61B3F53A),
    ParamInfo::new("air", "Mic Air", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x0001794A).with_vst3_id(0x0001794A).with_au_id(0x0001794A),
    // Equaliser
    ParamInfo::new("alleq", "All EQ", 0.0, 1.0, 1.0, Unit::Toggle)
        .with_clap_id(0x0589A20D).with_vst3_id(0x0589A20D).with_au_id(0x0589A20D),
    ParamInfo::new("filtroute", "Filters", 0.0, 3.0, 0.0, Unit::None).with_choices(ROUTES)
        .with_clap_id(0xCC2D7B9E).with_vst3_id(0x4C2D7B9E).with_au_id(0x4C2D7B9E),
    ParamInfo::new("hpf", "High Pass", 19.620285, 1506.9645, 19.620285, Unit::Hz).with_taper(Taper::Log)
        .with_clap_id(0x0001945E).with_vst3_id(0x0001945E).with_au_id(0x0001945E),
    ParamInfo::new("lpf", "Low Pass", 307.41495, 23611.451, 23611.451, Unit::Hz).with_taper(Taper::Log)
        .with_clap_id(0x0001A362).with_vst3_id(0x0001A362).with_au_id(0x0001A362),
    ParamInfo::new("shelfroute", "Shelving EQ", 0.0, 3.0, 0.0, Unit::None).with_choices(ROUTES)
        .with_clap_id(0xFA4B9E5F).with_vst3_id(0x7A4B9E5F).with_au_id(0x7A4B9E5F),
    ParamInfo::new("lfgain", "LF Gain", -17.5, 17.5, 0.0, Unit::Db)
        .with_clap_id(0xBE190BF9).with_vst3_id(0x3E190BF9).with_au_id(0x3E190BF9),
    ParamInfo::new("lffreq", "LF Frequency", 0.0, 3.0, 1.0, Unit::None).with_choices(&["20/33 Hz", "56/95 Hz", "160/270 Hz", "460/655 Hz"])
        .with_clap_id(0xBE18D6F2).with_vst3_id(0x3E18D6F2).with_au_id(0x3E18D6F2),
    ParamInfo::new("lfhigh", "LF Hi Range", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xBE199E1C).with_vst3_id(0x3E199E1C).with_au_id(0x3E199E1C),
    ParamInfo::new("hfgain", "HF Gain", -17.5, 17.5, 0.0, Unit::Db)
        .with_clap_id(0xB745A97D).with_vst3_id(0x3745A97D).with_au_id(0x3745A97D),
    ParamInfo::new("hffreq", "HF Frequency", 0.0, 3.0, 1.0, Unit::None).with_choices(&["1.5/2.2 kHz", "3.3/4.7 kHz", "6.8/10 kHz", "15/18 kHz"])
        .with_clap_id(0xB7457476).with_vst3_id(0x37457476).with_au_id(0x37457476),
    ParamInfo::new("hfhigh", "HF Hi Range", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xB7463BA0).with_vst3_id(0x37463BA0).with_au_id(0x37463BA0),
    ParamInfo::new("paramroute", "Parametric EQ", 0.0, 3.0, 0.0, Unit::None).with_choices(ROUTES)
        .with_clap_id(0x1C0CE9DC).with_vst3_id(0x1C0CE9DC).with_au_id(0x1C0CE9DC),
    ParamInfo::new("lmfgain", "LMF Gain", -14.8, 14.8, 0.0, Unit::Db)
        .with_clap_id(0x10EEE404).with_vst3_id(0x10EEE404).with_au_id(0x10EEE404),
    ParamInfo::new("lmffreq", "LMF Frequency", 35.4778, 405.566, 120.0, Unit::Hz).with_taper(Taper::Log)
        .with_clap_id(0x10EEAEFD).with_vst3_id(0x10EEAEFD).with_au_id(0x10EEAEFD),
    ParamInfo::new("lmfq", "LMF Q", 0.846151, 2.91361, 1.2, Unit::None).with_taper(Taper::Log)
        .with_clap_id(0x0032BE0C).with_vst3_id(0x0032BE0C).with_au_id(0x0032BE0C),
    ParamInfo::new("lmfx3", "LMF x3", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x06250480).with_vst3_id(0x06250480).with_au_id(0x06250480),
    ParamInfo::new("hmfgain", "HMF Gain", -14.8, 14.8, 0.0, Unit::Db)
        .with_clap_id(0x3D55F700).with_vst3_id(0x3D55F700).with_au_id(0x3D55F700),
    ParamInfo::new("hmffreq", "HMF Frequency", 540.156, 6001.71, 1800.0, Unit::Hz).with_taper(Taper::Log)
        .with_clap_id(0x3D55C1F9).with_vst3_id(0x3D55C1F9).with_au_id(0x3D55C1F9),
    ParamInfo::new("hmfq", "HMF Q", 0.851674, 2.91201, 1.2, Unit::None).with_taper(Taper::Log)
        .with_clap_id(0x0030EC90).with_vst3_id(0x0030EC90).with_au_id(0x0030EC90),
    ParamInfo::new("hmfx3", "HMF x3", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x05ECA67C).with_vst3_id(0x05ECA67C).with_au_id(0x05ECA67C),
    // Compressor
    ParamInfo::new("compin", "Comp In", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xAF3F3474).with_vst3_id(0x2F3F3474).with_au_id(0x2F3F3474),
    ParamInfo::new("comptype", "Comp Type", 0.0, 2.0, 0.0, Unit::None).with_choices(&["VCA", "Vintage Comp", "Vintage Lim"])
        .with_clap_id(0xDC491EC9).with_vst3_id(0x5C491EC9).with_au_id(0x5C491EC9),
    ParamInfo::new("ratio", "Ratio", 1.5, 10.0, 3.0, Unit::Ratio).with_taper(Taper::Log)
        .with_clap_id(0x0674500B).with_vst3_id(0x0674500B).with_au_id(0x0674500B),
    ParamInfo::new("threshold", "Comp Threshold", -28.0, 12.0, 0.0, Unit::Db)
        .with_clap_id(0xA3E1E46B).with_vst3_id(0x23E1E46B).with_au_id(0x23E1E46B),
    ParamInfo::new("attack", "Attack", 0.1, 100.0, 10.0, Unit::Ms).with_taper(Taper::Log)
        .with_clap_id(0xAC1EEE48).with_vst3_id(0x2C1EEE48).with_au_id(0x2C1EEE48),
    ParamInfo::new("release", "Release", 0.1, 7.0, 0.3, Unit::Seconds).with_taper(Taper::Log)
        .with_clap_id(0x41012807).with_vst3_id(0x41012807).with_au_id(0x41012807),
    ParamInfo::new("autorel", "Auto Release", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xD97A488A).with_vst3_id(0x597A488A).with_au_id(0x597A488A),
    ParamInfo::new("makeup", "Makeup", 0.0, 20.8278, 0.0, Unit::Db)
        .with_clap_id(0xBF895109).with_vst3_id(0x3F895109).with_au_id(0x3F895109),
    ParamInfo::new("compkey", "Comp Ext Key", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x38A760F0).with_vst3_id(0x38A760F0).with_au_id(0x38A760F0),
    ParamInfo::new("blend", "Blend", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x0597A051).with_vst3_id(0x0597A051).with_au_id(0x0597A051),
    ParamInfo::new("mix", "Mix", 0.0, 100.0, 100.0, Unit::Percent)
        .with_clap_id(0x0001A65C).with_vst3_id(0x0001A65C).with_au_id(0x0001A65C),
    ParamInfo::new("complisten", "Comp Listen", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xDFF5F596).with_vst3_id(0x5FF5F596).with_au_id(0x5FF5F596),
    // Gate
    ParamInfo::new("gatein", "Gate In", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xB5505330).with_vst3_id(0x35505330).with_au_id(0x35505330),
    ParamInfo::new("expand", "Expand", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xB328DE9A).with_vst3_id(0x3328DE9A).with_au_id(0x3328DE9A),
    ParamInfo::new("gatethresh", "Gate Threshold", -40.0, 10.0, -40.0, Unit::Db)
        .with_clap_id(0x343958C7).with_vst3_id(0x343958C7).with_au_id(0x343958C7),
    ParamInfo::new("range", "Range", 0.0, 80.0, 20.0, Unit::Db)
        .with_clap_id(0x0674393D).with_vst3_id(0x0674393D).with_au_id(0x0674393D),
    ParamInfo::new("hold", "Hold", 0.02, 4.0, 0.1, Unit::Seconds).with_taper(Taper::Log)
        .with_clap_id(0x0030F4BF).with_vst3_id(0x0030F4BF).with_au_id(0x0030F4BF),
    ParamInfo::new("gaterelease", "Gate Release", 0.1, 5.0, 0.5, Unit::Seconds).with_taper(Taper::Log)
        .with_clap_id(0xE3B1F0DC).with_vst3_id(0x63B1F0DC).with_au_id(0x63B1F0DC),
    ParamInfo::new("fastatt", "Fast Attack", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xBFCC74C5).with_vst3_id(0x3FCC74C5).with_au_id(0x3FCC74C5),
    ParamInfo::new("hyst", "Hysteresis", 0.0, 1.0, 1.0, Unit::Toggle)
        .with_clap_id(0x00311B32).with_vst3_id(0x00311B32).with_au_id(0x00311B32),
    ParamInfo::new("gatekey", "Gate Ext Key", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xF4BA19B4).with_vst3_id(0x74BA19B4).with_au_id(0x74BA19B4),
    ParamInfo::new("gatelisten", "Gate Listen", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x26A15652).with_vst3_id(0x26A15652).with_au_id(0x26A15652),
    // De-esser
    ParamInfo::new("deessin", "De-Ess In", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x5C0DE589).with_vst3_id(0x5C0DE589).with_au_id(0x5C0DE589),
    ParamInfo::new("deessthresh", "De-Ess Threshold", -30.0, 10.0, 0.0, Unit::Db)
        .with_clap_id(0x42C416A0).with_vst3_id(0x42C416A0).with_au_id(0x42C416A0),
    ParamInfo::new("deessfreq", "De-Ess Frequency", 2061.2, 8732.76, 5000.0, Unit::Hz).with_taper(Taper::Log)
        .with_clap_id(0x902965DC).with_vst3_id(0x102965DC).with_au_id(0x102965DC),
    ParamInfo::new("deesslisten", "De-Ess Listen", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x352C142B).with_vst3_id(0x352C142B).with_au_id(0x352C142B),
    // Routing and output
    ParamInfo::new("dynpos", "Dynamics Position", 0.0, 1.0, 0.0, Unit::None).with_choices(&["Post-EQ", "Pre-EQ"])
        .with_clap_id(0xB1816D3B).with_vst3_id(0x31816D3B).with_au_id(0x31816D3B),
    ParamInfo::new("bypass", "Bypass", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0xAE186CE8).with_vst3_id(0x2E186CE8).with_au_id(0x2E186CE8),
    ParamInfo::new("output", "Output", -60.0, 6.63557, 0.0, Unit::Db)
        .with_clap_id(0xC4111901).with_vst3_id(0x44111901).with_au_id(0x44111901),
    ParamInfo::new("softlimit", "Soft Limiter", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x853161D1).with_vst3_id(0x053161D1).with_au_id(0x053161D1),
    ParamInfo::new("mute", "Mute", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x00335219).with_vst3_id(0x00335219).with_au_id(0x00335219),
    ParamInfo::new("link", "Stereo Link", 0.0, 1.0, 1.0, Unit::Toggle)
        .with_clap_id(0x0032AFFA).with_vst3_id(0x0032AFFA).with_au_id(0x0032AFFA),
    // Processing
    ParamInfo::new("oversampling", "Oversampling", 0.0, 4.0, 1.0, Unit::None).with_choices(OVERSAMPLING)
        .with_clap_id(0xD4A4F8BB).with_vst3_id(0x54A4F8BB).with_au_id(0x54A4F8BB).with_flag(ParamFlags::PRESET_EXCLUDE),
    ParamInfo::new("offlineos", "Offline Oversampling", 0.0, 5.0, 0.0, Unit::None).with_choices(OFFLINE_OVERSAMPLING)
        .with_clap_id(0x7F662F07).with_vst3_id(0x7F662F07).with_au_id(0x7F662F07).with_flag(ParamFlags::PRESET_EXCLUDE),
    ParamInfo::new("hq", "HQ Mode (Offline)", 0.0, 1.0, 0.0, Unit::Toggle)
        .with_clap_id(0x00000D09).with_vst3_id(0x00000D09).with_au_id(0x00000D09).with_flag(ParamFlags::PRESET_EXCLUDE),
];

/// Table positions, named after the ids.
#[allow(dead_code)]
pub mod index {
    pub const SOURCE: usize = 0;
    pub const MICGAIN: usize = 1;
    pub const LINEGAIN: usize = 2;
    pub const TRIM: usize = 3;
    pub const PHASE: usize = 4;
    pub const IMPEDANCE: usize = 5;
    pub const AIR: usize = 6;
    pub const ALLEQ: usize = 7;
    pub const FILTROUTE: usize = 8;
    pub const HPF: usize = 9;
    pub const LPF: usize = 10;
    pub const SHELFROUTE: usize = 11;
    pub const LFGAIN: usize = 12;
    pub const LFFREQ: usize = 13;
    pub const LFHIGH: usize = 14;
    pub const HFGAIN: usize = 15;
    pub const HFFREQ: usize = 16;
    pub const HFHIGH: usize = 17;
    pub const PARAMROUTE: usize = 18;
    pub const LMFGAIN: usize = 19;
    pub const LMFFREQ: usize = 20;
    pub const LMFQ: usize = 21;
    pub const LMFX3: usize = 22;
    pub const HMFGAIN: usize = 23;
    pub const HMFFREQ: usize = 24;
    pub const HMFQ: usize = 25;
    pub const HMFX3: usize = 26;
    pub const COMPIN: usize = 27;
    pub const COMPTYPE: usize = 28;
    pub const RATIO: usize = 29;
    pub const THRESHOLD: usize = 30;
    pub const ATTACK: usize = 31;
    pub const RELEASE: usize = 32;
    pub const AUTOREL: usize = 33;
    pub const MAKEUP: usize = 34;
    pub const COMPKEY: usize = 35;
    pub const BLEND: usize = 36;
    pub const MIX: usize = 37;
    pub const COMPLISTEN: usize = 38;
    pub const GATEIN: usize = 39;
    pub const EXPAND: usize = 40;
    pub const GATETHRESH: usize = 41;
    pub const RANGE: usize = 42;
    pub const HOLD: usize = 43;
    pub const GATERELEASE: usize = 44;
    pub const FASTATT: usize = 45;
    pub const HYST: usize = 46;
    pub const GATEKEY: usize = 47;
    pub const GATELISTEN: usize = 48;
    pub const DEESSIN: usize = 49;
    pub const DEESSTHRESH: usize = 50;
    pub const DEESSFREQ: usize = 51;
    pub const DEESSLISTEN: usize = 52;
    pub const DYNPOS: usize = 53;
    pub const BYPASS: usize = 54;
    pub const OUTPUT: usize = 55;
    pub const SOFTLIMIT: usize = 56;
    pub const MUTE: usize = 57;
    pub const LINK: usize = 58;
    pub const OVERSAMPLING: usize = 59;
    pub const OFFLINEOS: usize = 60;
    pub const HQ: usize = 61;
}

/// With the `qa-identity` feature the build is another plugin to a host ("Blue Channel Port
/// Test"), so it can be installed for host tests beside the release it would otherwise replace.
#[cfg(not(feature = "qa-identity"))]
const IDENTITY: (&str, &str) = ("com.opengrid.bluechannel", "Blue Channel");
#[cfg(feature = "qa-identity")]
const IDENTITY: (&str, &str) = ("com.opengrid.bluechannel.porttest", "Blue Channel Port Test");

pub static INFO: PluginInfo =
    PluginInfo::new(IDENTITY.0, IDENTITY.1, "OpenGrid", env!("CARGO_PKG_VERSION"), Category::Dynamics, &PARAMS).with_editor(true);
