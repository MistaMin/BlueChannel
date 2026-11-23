//! Blue Channel in its own window.
//!
//!   blue-channel-app            a built-in plucked pattern through the strip, to the default output
//!   blue-channel-app --input    the default input device through the strip (use headphones)
//!   blue-channel-app --silent   the window and the engine, without opening an audio device
//!
//! The input is not live by default: a microphone next to loudspeakers would feed back.

use std::cell::Cell;
use std::rc::Rc;
use std::sync::Arc;

use blue_channel::{make_editor, BlueChannel};
use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use openplugin::core::editor::{Gesture, GestureQueue};
use openplugin::core::spsc;
use openplugin::core::EditorHost;
use openplugin::prelude::*;

const MAX_BLOCK: usize = 4096;

struct Voice {
    plugin: BlueChannel,
    gestures: Arc<GestureQueue>,
    io: Vec<Vec<f32>>,
    frame: u64,
    rate: f32,
}

impl Voice {
    fn new(plugin: BlueChannel, gestures: Arc<GestureQueue>, rate: f32) -> Self {
        let mut voice = Self { plugin, gestures, io: vec![vec![0.0; MAX_BLOCK]; 2], frame: 0, rate };
        voice.plugin.prepare(rate, MAX_BLOCK, 2);
        voice.plugin.reset();
        voice
    }

    /// One block: the demo pattern, or what `input` holds (interleaved stereo).
    fn render(&mut self, frames: usize, input: Option<&mut spsc::Consumer<f32>>) {
        while let Some(gesture) = self.gestures.pop() {
            if let Gesture::Perform(index, value) = gesture {
                self.plugin.set_param_index(index as usize, value);
            }
        }
        match input {
            Some(input) => {
                for i in 0..frames {
                    self.io[0][i] = input.pop().unwrap_or(0.0);
                    self.io[1][i] = input.pop().unwrap_or(0.0);
                }
            }
            None => {
                for i in 0..frames {
                    let t = self.frame as f32 / self.rate;
                    let step = (t * 2.0) as usize;
                    let beat = (t * 2.0).fract();
                    let note = [110.0, 164.8, 146.8, 220.0, 196.0, 130.8, 246.9, 123.5][step % 8];
                    let envelope = (-beat * 9.0).exp() * if step % 4 == 3 { 0.0 } else { 1.0 };
                    let w = t * note * std::f32::consts::TAU;
                    let x = 0.2 * envelope * (w.sin() + 0.4 * (2.0 * w).sin() + 0.15 * (3.01 * w).sin());
                    self.io[0][i] = x;
                    self.io[1][i] = x;
                    self.frame += 1;
                }
            }
        }
        self.plugin.process(&mut ChannelsMut::new(&mut self.io), frames);
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let silent = args.iter().any(|a| a == "--silent");
    let live = args.iter().any(|a| a == "--input");

    let plugin = BlueChannel::default();
    let state = plugin.editor_state();
    let gestures = Arc::new(GestureQueue::new(4096));
    let sink = gestures.clone();
    // The editor asks for another window when the skin changes (the two skins have different
    // shapes). The request is noted here and carried out before the next tick.
    let asked: Rc<Cell<Option<(u32, u32)>>> = Rc::default();
    let note = asked.clone();
    let host = EditorHost::new(move |gesture| {
        sink.push(gesture);
    })
    .with_resize(move |width, height| {
        note.set(Some((width, height)));
        true
    });
    let mut editor = make_editor(host).expect("the editor");
    editor.bind(state);

    let mut streams = Vec::new();
    if silent {
        let mut voice = Voice::new(plugin, gestures, 48_000.0);
        std::thread::spawn(move || loop {
            voice.render(480, None);
            std::thread::sleep(std::time::Duration::from_millis(10));
        });
    } else {
        let host = cpal::default_host();
        let device = host.default_output_device().expect("no audio output device");
        let config = device.default_output_config().expect("no output format");
        let channels = config.channels() as usize;
        let mut voice = Voice::new(plugin, gestures, config.sample_rate().0 as f32);
        let mut input = None;
        if live {
            let device = host.default_input_device().expect("no audio input device");
            let config = device.default_input_config().expect("no input format");
            let width = config.channels() as usize;
            let (mut tx, rx) = spsc::channel::<f32>(1 << 16);
            let stream = device
                .build_input_stream(
                    &config.into(),
                    move |data: &[f32], _: &cpal::InputCallbackInfo| {
                        // Interleaved stereo, whatever the device's width; a full queue drops the rest.
                        for frame in data.chunks(width) {
                            if tx.push(frame[0]).is_err() || tx.push(frame[width.min(2) - 1]).is_err() {
                                break;
                            }
                        }
                    },
                    |e| eprintln!("input stream: {e}"),
                    None,
                )
                .expect("input stream");
            stream.play().expect("start input");
            streams.push(stream);
            input = Some(rx);
        }
        let stream = device
            .build_output_stream(
                &config.into(),
                move |data: &mut [f32], _: &cpal::OutputCallbackInfo| {
                    for block in data.chunks_mut(MAX_BLOCK * channels) {
                        let frames = block.len() / channels;
                        voice.render(frames, input.as_mut());
                        for (i, frame) in block.chunks_mut(channels).enumerate() {
                            for (ch, sample) in frame.iter_mut().enumerate() {
                                *sample = if ch < 2 { voice.io[ch][i] } else { 0.0 };
                            }
                        }
                    }
                },
                |e| eprintln!("output stream: {e}"),
                None,
            )
            .expect("output stream");
        stream.play().expect("start output");
        streams.push(stream);
    }

    let poll: openplugin::dri::cocoa::Poll = Box::new(move |editor| {
        if let Some((width, height)) = asked.take() {
            window::shape(&editor.size_range(), width, height);
        }
    });
    let code = openplugin::dri::cocoa::run_window("Blue Channel", editor, Some(poll));
    drop(streams);
    std::process::exit(code);
}

/// The app's window, through the Objective-C runtime.
mod window {
    use std::ffi::{c_char, c_void};

    use openplugin::core::editor::SizeRange;

    type Id = *mut c_void;

    #[repr(C)]
    #[derive(Clone, Copy)]
    struct Size {
        width: f64,
        height: f64,
    }

    #[link(name = "objc")]
    extern "C" {
        fn objc_getClass(name: *const c_char) -> Id;
        fn sel_registerName(name: *const c_char) -> Id;
        fn objc_msgSend();
    }
    #[link(name = "AppKit", kind = "framework")]
    extern "C" {}

    unsafe fn get(object: Id, selector: &[u8]) -> Id {
        let send: unsafe extern "C" fn(Id, Id) -> Id = std::mem::transmute(objc_msgSend as unsafe extern "C" fn());
        send(object, sel_registerName(selector.as_ptr().cast()))
    }

    unsafe fn set(object: Id, selector: &[u8], size: (u32, u32)) {
        let send: unsafe extern "C" fn(Id, Id, Size) = std::mem::transmute(objc_msgSend as unsafe extern "C" fn());
        send(object, sel_registerName(selector.as_ptr().cast()), Size { width: f64::from(size.0), height: f64::from(size.1) });
    }

    /// Give the window the limits and shape of `range` and a content of `width` x `height` points.
    pub fn shape(range: &SizeRange, width: u32, height: u32) {
        unsafe {
            let app = get(objc_getClass(c"NSApplication".as_ptr()), b"sharedApplication\0");
            let window = if app.is_null() { app } else { get(get(app, b"windows\0"), b"firstObject\0") };
            if window.is_null() {
                return;
            }
            set(window, b"setContentMinSize:\0", range.min);
            set(window, b"setContentMaxSize:\0", range.max);
            if let Some(aspect) = range.aspect_ratio() {
                set(window, b"setContentAspectRatio:\0", aspect);
            }
            set(window, b"setContentSize:\0", (width, height));
        }
    }
}
