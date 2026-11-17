//! What a frame of the editor costs: `cargo run --release --example ui_bench`.
//! Each page at 0.8 of its panel (1248 x 360 points analogue, 1000 x 720 Digital) at 2x, with the meters moving as they do while audio plays.
use std::sync::Arc;
use std::time::Instant;

use blue_channel::{BlueEditor, Shared, Telemetry};
use openplugin::core::{Editor, EditorHost};

fn main() {
    for (name, skin, details) in [("analogue", 0u8, false), ("digital overview", 1, false), ("digital all controls", 1, true)] {
        let mut editor = BlueEditor::new(EditorHost::detached());
        let shared = Arc::new(Shared::default());
        editor.bind(Some(shared.clone()));
        editor.show(skin, details);
        let (width, height) = editor.page().size();
        editor.resize((width * 0.8).round() as u32, (height * 0.8).round() as u32);
        let start = Instant::now();
        editor.render(2.0);
        let first = start.elapsed().as_secs_f64() * 1e3;
        let (mut idle, mut playing) = (0.0, 0.0);
        let frames = 90;
        for moving in [false, true] {
            let start = Instant::now();
            for n in 0..frames {
                if moving {
                    let level = 0.1 + 0.4 * (n as f32 * 0.21).sin().abs();
                    let noise: Vec<f32> = (0..1600).map(|i| level * ((i * 7919 + n * 104729) % 1000) as f32 / 1000.0 - level * 0.5).collect();
                    shared.spectrum.push(&[&noise]);
                    shared.publish(
                        &Telemetry { input_peak: [level, level], output_peak: [level * 1.5, level], input_square: [level * level * 0.5, 0.0], output_square: [level * level, 0.0], comp_gr: 6.0 * level, gate_gr: 30.0 * level, ..Telemetry::default() },
                        2,
                        false,
                    );
                }
                editor.tick(1.0 / 30.0);
                let _ = editor.damage();
                editor.render(2.0);
            }
            let per_frame = start.elapsed().as_secs_f64() * 1e3 / frames as f64;
            if moving { playing = per_frame } else { idle = per_frame }
        }
        println!("{name:22} first frame {first:7.1} ms, idle {idle:6.3} ms/frame, meters moving {playing:6.2} ms/frame ({:.0} % of a core at 30 frames a second)", playing * 30.0 / 10.0);
    }
}
