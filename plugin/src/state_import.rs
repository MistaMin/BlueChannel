//! Reading the state the JUCE release of Blue Channel saved, so the sessions
//! and presets made with it open in the port (`openplugin_core::foreign_state`).
//!
//! The format (`openplugin-port capture`, tests/fixtures): the magic `VC2!`, a
//! little-endian `u32` length, then an XML document and a zero byte. The root
//! is `BlueChannel` with one `PARAM` element per parameter: `id` is the
//! parameter id, which the port kept, and `value` the plain value.

use openplugin::core::foreign_state::import::{import, ImportedState, ParamMap, StateFormat};
use openplugin::core::foreign_state::{xml, ErrorKind, ForeignError, Result};

use crate::{EditorSettings, COUNT, PARAMS};

const MAGIC: &[u8; 4] = b"VC2!";
const ROOT: &str = "BlueChannel";

pub static FORMATS: [StateFormat; 1] = [StateFormat { name: "Blue Channel 1.0 (JUCE)", detect: original_detect, decode: original_decode }];

fn invalid(what: &'static str) -> ForeignError {
    ForeignError { kind: ErrorKind::Invalid, offset: 0, what }
}

fn original_detect(data: &[u8]) -> Option<u32> {
    data.starts_with(MAGIC).then_some(1)
}

fn document(data: &[u8]) -> Option<&[u8]> {
    let length = data.get(4..8).map(|b| u32::from_le_bytes([b[0], b[1], b[2], b[3]]) as usize)?;
    let body = data.get(8..)?.get(..length)?;
    Some(body.strip_suffix(&[0]).unwrap_or(body))
}

fn original_decode(data: &[u8], version: u32) -> Result<ImportedState> {
    let root = xml::parse(document(data).ok_or(invalid("shorter than its length"))?)?;
    if root.name != ROOT {
        return Err(invalid("not a Blue Channel state"));
    }
    let mut state = ImportedState::new(FORMATS[0].name, version);
    for param in root.children_named("PARAM") {
        let id = param.attr("id").ok_or(invalid("a PARAM without an id"))?;
        let value = param.attr("value").and_then(|v| v.parse::<f64>().ok()).filter(|v| v.is_finite()).ok_or(invalid("a PARAM without a number"))?;
        state.push_plain(id.into(), value);
    }
    Ok(state)
}

/// `None` when `data` is not the release's format (the port's own is tried
/// next); `Some(Err(_))` when it is but is damaged (refuse it); otherwise every
/// parameter's plain value and the editor's settings. As in the release, a
/// parameter the state does not name gets its default, and one this build does
/// not have is ignored.
pub fn import_original(data: &[u8]) -> Option<std::result::Result<([f32; COUNT], EditorSettings), String>> {
    let state = match import(&FORMATS, data)? {
        Ok(state) => state,
        Err(e) => return Some(Err(e.to_string())),
    };
    let mut map = ParamMap::new();
    for info in PARAMS.iter() {
        map.map(info.id, info.id);
    }
    let mut values: [f32; COUNT] = std::array::from_fn(|i| PARAMS[i].default);
    for (index, value) in map.apply(&PARAMS, &state).values {
        values[index] = value;
    }
    Some(Ok((values, settings(data))))
}

/// The editor's settings are attributes of the root element: `skin`,
/// `vusource`, `vucal`, and `uiscale` once `uiscaleversion` is 3.
fn settings(data: &[u8]) -> EditorSettings {
    let mut e = EditorSettings::default();
    let Some(root) = document(data).and_then(|d| xml::parse(d).ok()) else { return e };
    let number = |name: &str| root.attr(name).and_then(|v| v.parse::<f32>().ok()).filter(|v| v.is_finite());
    if let Some(v) = number("skin") {
        e.skin = u8::from(v >= 0.5);
    }
    if let Some(v) = number("vusource") {
        e.vu_source = v.clamp(0.0, 2.0).round() as u8;
    }
    if let Some(v) = number("vucal") {
        e.vu_high_cal = v >= 0.5;
    }
    if let (Some(v), Some(version)) = (number("uiscale"), number("uiscaleversion")) {
        if version >= 3.0 {
            e.ui_scale = v.clamp(0.5, 2.0);
        }
    }
    e
}
