//! Writes the editor to a PPM: `snapshot out.ppm [analogue|overview|controls] [scale] [param=value ...]`.
//! `size=0.8` among the settings gives the window that many points per panel unit (0.5 to 2;
//! the editor opens at 0.8, and the default here is the full panel, 1). `hover=ratio` points
//! the mouse at that parameter's control.
use blue_channel::{BlueEditor, PARAMS};

use openplugin::core::{Editor, EditorHost};

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let Some(out) = args.get(1) else {
        eprintln!("usage: snapshot <out.ppm> [analogue|overview|controls] [scale] [size=0.8] [hover=id] [param=value ...]");
        std::process::exit(2);
    };
    let page = args.get(2).map_or("analogue", String::as_str);
    let scale: f32 = args.get(3).and_then(|s| s.parse().ok()).unwrap_or(1.0);
    let mut editor = BlueEditor::new(EditorHost::detached());
    let size = args.iter().skip(4).find_map(|a| a.strip_prefix("size=")).map_or(1.0, |s| s.parse::<f32>().expect("a size"));
    for (i, p) in PARAMS.iter().enumerate() {
        editor.param_changed(i, p.default);
    }
    let hover = args.iter().skip(4).find_map(|a| a.strip_prefix("hover="));
    for setting in args.iter().skip(4).filter(|a| !a.starts_with("size=") && !a.starts_with("hover=")) {
        let (id, value) = setting.split_once('=').expect("param=value");
        let index = PARAMS.iter().position(|p| p.id == id).expect("a parameter id");
        editor.param_changed(index, value.parse().expect("a number"));
    }
    editor.show(u8::from(page != "analogue"), page == "controls");
    // The window of that page (the analogue panel is 1560 x 450 units, the Digital pages 1250 x 900).
    let (width, height) = editor.page().size();
    editor.resize((width * size).round() as u32, (height * size).round() as u32);
    if let Some(id) = hover {
        // Point at the control of a parameter: its value shows.
        let index = PARAMS.iter().position(|p| p.id == id).expect("a parameter id");
        let (rect, _) = editor.widget_params().into_iter().find(|(_, params)| params.first() == Some(&index)).expect("a control on this page");
        // A knob's centre is a little under the middle of its rectangle, between caption and legends.
        editor.mouse_move((rect.0 + rect.2 * 0.5) * size, (rect.1 + rect.3 * 0.5 + 1.0) * size);
    }
    editor.tick(1.0 / 30.0);
    let frame = editor.render(scale);
    let mut ppm = format!("P6\n{} {}\n255\n", frame.width, frame.height).into_bytes();
    for p in frame.pixels {
        ppm.extend_from_slice(&[(p >> 16) as u8, (p >> 8) as u8, *p as u8]);
    }
    std::fs::write(out, ppm).expect("write snapshot");
}
