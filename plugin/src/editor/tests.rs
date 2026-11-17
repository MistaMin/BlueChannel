use std::cell::RefCell;
use std::rc::Rc;

use openplugin::core::editor::Gesture;

use super::*;
use crate::engine::Telemetry;

type Log = Rc<RefCell<Vec<Gesture>>>;

fn editor() -> (BlueEditor, Log) {
    let log: Log = Rc::default();
    let sink = log.clone();
    let mut editor = BlueEditor::new(EditorHost::new(move |g| sink.borrow_mut().push(g)));
    editor.resize(1560, 450);
    (editor, log)
}

/// An editor showing a page in a window of the page's own size, a point to a panel unit.
fn showing(skin: u8, details: bool) -> (BlueEditor, Log) {
    let (mut e, log) = editor();
    e.show(skin, details);
    let (width, height) = e.page().size();
    e.resize(width as u32, height as u32);
    assert_eq!(e.size(), (width as u32, height as u32));
    (e, log)
}

fn params_of(log: &Log) -> Vec<usize> {
    let mut params: Vec<usize> = log
        .borrow()
        .iter()
        .filter_map(|g| match g {
            Gesture::Begin(p) | Gesture::End(p) | Gesture::Perform(p, _) => Some(*p as usize),
            _ => None,
        })
        .collect();
    params.sort_unstable();
    params.dedup();
    params
}

/// Every begin has its end, in order, per parameter.
fn balanced(log: &Log) -> bool {
    let mut open = Vec::new();
    for g in log.borrow().iter() {
        match g {
            Gesture::Begin(p) => open.push(*p),
            Gesture::End(p) => {
                let Some(at) = open.iter().position(|o| o == p) else { return false };
                open.remove(at);
            }
            _ => {}
        }
    }
    open.is_empty()
}

fn pixels(editor: &mut BlueEditor) -> Vec<u32> {
    editor.render(1.0).pixels.to_vec()
}

fn pages() -> [(u8, bool, Page); 3] {
    [(0, false, Page::Analogue), (1, false, Page::Overview), (1, true, Page::Controls)]
}

#[test]
fn every_page_renders_at_the_panel_size_and_at_smaller_sizes() {
    for (skin, details, page) in pages() {
        let (mut e, _) = showing(skin, details);
        assert_eq!(e.page(), page);
        let (width, height) = (page.size().0 as usize, page.size().1 as usize);
        let frame = e.render(1.0);
        assert_eq!((frame.width, frame.height), (width, height));
        let first = frame.pixels[0];
        assert!(frame.pixels.iter().any(|p| *p != first));
        let small = (width as u32 * 4 / 5, height as u32 * 4 / 5);
        e.resize(small.0, small.1);
        assert_eq!(e.size(), small);
        let frame = e.render(2.0);
        assert_eq!((frame.width, frame.height), (small.0 as usize * 2, small.1 as usize * 2), "{page:?} at 2x");
        // On a 1x display the frame is the window's size too, not the panel's.
        let frame = e.render(1.0);
        assert_eq!((frame.width, frame.height), (small.0 as usize, small.1 as usize), "{page:?} at 1x");
    }
}

/// The visual sweep: moving any parameter from the host changes what the
/// analogue panel and the Digital "all controls" page show.
#[test]
fn every_parameter_is_shown_on_both_skins() {
    for (skin, details, page) in [(0, false, Page::Analogue), (1, true, Page::Controls)] {
        for (index, info) in PARAMS.iter().enumerate() {
            let (mut e, _) = showing(skin, details);
            // Line gain shows once the input is not Mic.
            if index == ix::LINEGAIN {
                e.param_changed(ix::SOURCE, 1.0);
            }
            let before = pixels(&mut e);
            let other = if info.default == info.max { info.min } else { info.max };
            e.param_changed(index, other);
            assert_ne!(before, pixels(&mut e), "{} does not show on {page:?}", info.id);
        }
    }
}

/// Operating a control sends gestures for its own parameter and no other,
/// and every gesture it opens is closed.
#[test]
fn every_control_drives_its_own_parameter() {
    for (skin, details, page) in pages() {
        for source in 0..3 {
            let (mut e, log) = showing(skin, details);
            e.param_changed(ix::SOURCE, source as f32);
            let count = e.widget_params().len();
            for n in 0..count {
                let (rect, params) = e.widget_params()[n].clone();
                if params.is_empty() || matches!(e.widgets[n].kind, Kind::EqGraph | Kind::CompGraph) {
                    continue;
                }
                // Where it answers: its control's centre, or its own first part.
                let (x, y) = match e.ctl[n].and_then(|id| e.controls.get(id)) {
                    Some(control) => control.area.center(),
                    None if matches!(e.widgets[n].kind, Kind::Knob { enabled: false, .. } | Kind::Hero { enabled: false, .. } | Kind::Slider { enabled: false, .. }) => continue,
                    None => (rect.0 + rect.2 * 0.8, rect.1 + rect.3 * 0.5),
                };
                log.borrow_mut().clear();
                e.mouse_down(x, y, 1);
                e.mouse_move(x + 3.0, y - 30.0);
                e.mouse_up(x + 3.0, y - 30.0);
                let touched = params_of(&log);
                assert!(!touched.is_empty(), "{page:?} widget {n} ({}) sent nothing", PARAMS[params[0]].id);
                assert!(touched.iter().all(|p| params.contains(p)), "{page:?} widget {n} ({}) moved {touched:?}", PARAMS[params[0]].id);
                assert!(balanced(&log), "{page:?} widget {n}: {:?}", log.borrow());
                // Undo what the click did, so the page stays as it was for the next widget.
                for p in params {
                    e.param_changed(p, PARAMS[p].default);
                }
                e.param_changed(ix::SOURCE, source as f32);
                assert_eq!(e.widget_params().len(), count, "the page changed under the sweep");
            }
        }
    }
}

fn centre_of(e: &BlueEditor, param: usize) -> (f32, f32, f32, f32) {
    e.widget_params().into_iter().find(|(_, p)| p.first() == Some(&param)).map(|(r, _)| r).unwrap_or_else(|| panic!("no widget for {}", PARAMS[param].id))
}

#[test]
fn a_led_key_toggles_and_a_knob_drag_is_one_gesture() {
    let (mut e, log) = editor();
    let (x, y, w, h) = centre_of(&e, ix::COMPIN);
    e.mouse_down(x + w * 0.5, y + h * 0.5, 1);
    e.mouse_up(x + w * 0.5, y + h * 0.5);
    assert_eq!(e.values()[ix::COMPIN], 1.0);
    assert!(log.borrow().contains(&Gesture::Perform(ix::COMPIN as u32, 1.0)));

    log.borrow_mut().clear();
    let id = e.ctl.iter().flatten().copied().find(|id| e.controls.get(*id).unwrap().param == ix::OUTPUT).unwrap();
    let (cx, cy) = e.controls.get(id).unwrap().area.center();
    e.mouse_down(cx, cy, 1);
    e.mouse_move(cx, cy + 60.0);
    e.mouse_up(cx, cy + 60.0);
    let seen = log.borrow().clone();
    assert_eq!(seen.first(), Some(&Gesture::Begin(ix::OUTPUT as u32)));
    assert_eq!(seen.last(), Some(&Gesture::End(ix::OUTPUT as u32)));
    assert!(e.values()[ix::OUTPUT] < 0.0, "dragging down lowers the output: {}", e.values()[ix::OUTPUT]);
}

#[test]
fn the_route_keys_and_the_selectors_work_as_on_the_release() {
    let (mut e, _) = editor();
    // Route: In, then Comp, then Comp again returns to Out.
    let (x, y, w, h) = centre_of(&e, ix::FILTROUTE);
    let third = |k: f32| (x + w * (k + 0.5) / 3.0, y + h * 0.5);
    for (k, want) in [(0.0, 1.0), (1.0, 2.0), (1.0, 0.0), (2.0, 3.0)] {
        let (px, py) = third(k);
        e.mouse_down(px, py, 1);
        e.mouse_up(px, py);
        assert_eq!(e.values()[ix::FILTROUTE], want);
    }
    // The Oversample key steps through the factors and wraps.
    let (x, y, _, h) = centre_of(&e, ix::OVERSAMPLING);
    let seen: Vec<f32> = (0..5)
        .map(|_| {
            e.mouse_down(x + 20.0, y + h * 0.5, 1);
            e.mouse_up(x + 20.0, y + h * 0.5);
            e.values()[ix::OVERSAMPLING]
        })
        .collect();
    assert_eq!(seen, [2.0, 3.0, 4.0, 0.0, 1.0]);
    // The Offline key ends on HQ, which is its own parameter, and leaves it again.
    let (x, y, _, h) = centre_of(&e, ix::OFFLINEOS);
    let mut seen = Vec::new();
    for _ in 0..8 {
        e.mouse_down(x + 20.0, y + h * 0.5, 1);
        e.mouse_up(x + 20.0, y + h * 0.5);
        seen.push((e.values()[ix::OFFLINEOS], e.values()[ix::HQ]));
    }
    assert_eq!(seen, [(1.0, 0.0), (2.0, 0.0), (3.0, 0.0), (4.0, 0.0), (5.0, 0.0), (5.0, 1.0), (0.0, 0.0), (1.0, 0.0)]);
}

#[test]
fn the_gain_control_follows_the_selected_input() {
    let (mut e, _) = editor();
    let has = |e: &BlueEditor, p: usize| e.widget_params().iter().any(|(_, params)| params.contains(&p));
    assert!(has(&e, ix::MICGAIN) && !has(&e, ix::LINEGAIN));
    e.param_changed(ix::SOURCE, 1.0);
    assert!(!has(&e, ix::MICGAIN) && has(&e, ix::LINEGAIN));
    // Instrument: the Line gain control is shown but does nothing.
    e.param_changed(ix::SOURCE, 2.0);
    let n = e.widgets.iter().position(|w| w.params() == [ix::LINEGAIN]).unwrap();
    assert!(e.ctl[n].is_none());
}

#[test]
fn nothing_is_repainted_until_something_changes() {
    for (skin, details, _) in [(0, false, Page::Analogue), (1, true, Page::Controls)] {
        let (mut e, _) = showing(skin, details);
        e.render(1.0);
        assert_eq!(e.damage(), Some(Vec::new()));
        e.param_changed(ix::MUTE, 1.0);
        let damage = e.damage().expect("a partial repaint");
        assert_eq!(damage.len(), 1, "{damage:?}");
        assert!(damage[0].w < 120.0 && damage[0].h < 80.0, "{damage:?}");
        // A partial repaint gives the picture a full one gives.
        let partial = pixels(&mut e);
        e.damage.add_all();
        assert_eq!(partial, pixels(&mut e));
    }
}

#[test]
fn the_meters_follow_the_engine_and_the_settings_are_shared() {
    let (mut e, _) = editor();
    let shared = Arc::new(Shared::default());
    e.bind(Some(shared.clone()));
    let idle = pixels(&mut e);
    let telemetry = Telemetry { input_peak: [0.5, 0.25], output_peak: [1.2, 0.1], input_square: [0.05, 0.0], output_square: [0.1, 0.0], comp_gr: 6.0, gate_gr: 22.0, deess_led: 1.5, overload: 1, ..Telemetry::default() };
    shared.publish(&telemetry, 2, true);
    e.tick(1.0 / 30.0);
    assert!((e.meters.input_db - 20.0 * 0.5f32.log10()).abs() < 1e-3 && e.meters.output_db > 0.0);
    assert_eq!((e.meters.comp_gr, e.meters.gate_gr, e.meters.deess, e.meters.overload, e.meters.key_input), (6.0, 22.0, 1.0, 1.0, true));
    // The input reads in dBu against +4 dBu = 0 VU: sqrt(0.05) of +22 dBu full-scale peak, as RMS.
    let dbu = 20.0 * (0.05f32.sqrt() * FULL_SCALE_PEAK / DBU_VOLTS).log10();
    assert!((e.meters.vu - (dbu - 4.0)).abs() < 1e-3, "{}", e.meters.vu);
    assert!(e.meters.needle > 0.0);
    assert_ne!(idle, pixels(&mut e));
    // The peaks were taken; the overload lamp fades over twenty ticks.
    shared.publish(&Telemetry::default(), 2, false);
    e.tick(1.0 / 30.0);
    assert!(e.meters.input_db < -100.0 && (e.meters.overload - 0.95).abs() < 1e-4 && !e.meters.key_input);

    // The Meter key steps the source, the 0 VU key the calibration; both live in the shared state.
    let rects: Vec<_> = e.widgets.iter().filter(|w| matches!(w.kind, Kind::PropStep { .. })).map(|w| w.rect).collect();
    for r in &rects {
        e.mouse_down(r.0 + 20.0, r.1 + 8.0, 1);
        e.mouse_up(r.0 + 20.0, r.1 + 8.0);
    }
    let s = shared.settings();
    assert_eq!((s.vu_source, s.vu_high_cal), (1, true));
    // With Comp GR as the source the needle reads the reduction.
    shared.publish(&telemetry, 2, true);
    e.tick(1.0 / 30.0);
    assert_eq!(e.meters.vu, -6.0);
}

#[test]
fn the_skin_and_the_size_are_kept_in_the_shared_state() {
    let (mut e, _) = editor();
    let shared = Arc::new(Shared::default());
    e.bind(Some(shared.clone()));
    let key = e.widgets.iter().find(|w| matches!(w.kind, Kind::SkinKey)).unwrap().rect;
    e.mouse_down(key.0 + 20.0, key.1 + 8.0, 1);
    e.mouse_up(key.0 + 20.0, key.1 + 8.0);
    assert_eq!((e.page(), shared.settings().skin), (Page::Overview, 1));
    // ANALOGUE takes it back. No host resized the window here, so the Digital page is fitted
    // into the analogue panel's and the button is where that puts it.
    let back = e.widgets.iter().find(|w| matches!(w.kind, Kind::Button { action: Action::Skin(0), .. })).unwrap().rect;
    let at = e.to_window((back.0 + 20.0, back.1 + 8.0, 0.0, 0.0));
    e.mouse_down(at.x, at.y, 1);
    e.mouse_up(at.x, at.y);
    assert_eq!((e.page(), shared.settings().skin), (Page::Analogue, 0));
    e.resize(1872, 540);
    assert_eq!(shared.settings().ui_scale, 1.2);
    // A session saved in the Digital skin opens in it.
    shared.set_settings(EditorSettings { skin: 1, ..shared.settings() });
    let (mut other, _) = editor();
    other.bind(Some(shared));
    assert_eq!(other.page(), Page::Overview);
}

#[test]
fn hits_follow_the_size_of_the_window() {
    let (mut e, _) = editor();
    e.resize(780, 225);
    let (x, y, w, h) = centre_of(&e, ix::MUTE);
    // Where Mute is at half size, and where it was at full size (nothing there now).
    e.mouse_down((x + w * 0.5) * 0.5, (y + h * 0.5) * 0.5, 1);
    e.mouse_up((x + w * 0.5) * 0.5, (y + h * 0.5) * 0.5);
    assert_eq!(e.values()[ix::MUTE], 1.0);
    e.mouse_down(x + w * 0.5, y + h * 0.5, 1);
    e.mouse_up(x + w * 0.5, y + h * 0.5);
    assert_eq!(e.values()[ix::MUTE], 1.0);
}

#[test]
fn dragging_a_graph_handle_is_one_gesture_per_parameter() {
    let (mut e, log) = showing(1, false);
    e.render(1.0); // computes the response, which places the handles
    let r = e.widgets.iter().find(|w| matches!(w.kind, Kind::EqGraph)).unwrap().rect;
    let a = graphs::eq_plot(r);
    let (hx, hy) = e.eq.handle(&a, 2);
    e.mouse_down(hx, hy, 1);
    e.mouse_move(graphs::eq_x(&a, 3000.0), graphs::eq_y(&a, 6.0));
    e.mouse_move(graphs::eq_x(&a, 4000.0), graphs::eq_y(&a, 9.0));
    e.mouse_up(0.0, 0.0);
    assert_eq!(params_of(&log), [ix::HMFGAIN, ix::HMFFREQ]);
    assert!(balanced(&log));
    assert_eq!(log.borrow().iter().filter(|g| matches!(g, Gesture::Begin(_))).count(), 2);
    assert!((e.values()[ix::HMFGAIN] - 9.0).abs() < 0.05 && (e.values()[ix::HMFFREQ] - 4000.0).abs() < 20.0, "{:?}", (e.values()[ix::HMFGAIN], e.values()[ix::HMFFREQ]));
    // The wheel over a mid band sets its Q.
    let (hx, hy) = e.eq.handle(&a, 2);
    let _ = (hx, hy);
    e.render(1.0);
    let (hx, hy) = e.eq.handle(&a, 2);
    e.wheel(hx, hy, 0.5);
    assert!((e.values()[ix::HMFQ] - 1.7).abs() < 1e-4);

    log.borrow_mut().clear();
    let r = e.widgets.iter().find(|w| matches!(w.kind, Kind::CompGraph)).unwrap().rect;
    let a = graphs::comp_plot(r);
    let (hx, hy) = CompCurve::new(e.values()).handle(&a, 0);
    e.mouse_down(hx, hy, 1);
    e.mouse_move(graphs::comp_x(&a, -12.0), hy);
    e.mouse_up(0.0, 0.0);
    assert_eq!(params_of(&log), [ix::THRESHOLD]);
    assert!(balanced(&log) && (e.values()[ix::THRESHOLD] + 12.0).abs() < 0.05);
}

/// What `reach` promises: with the meters moving, lamps lighting, keys and knobs pointed at
/// and values changing, repainting only the damage gives the picture a full repaint gives.
/// At the opening size, where a panel unit is 0.8 points, on a 1x and on a 2x display.
#[test]
fn a_partial_repaint_is_a_full_repaint_at_the_opening_size() {
    for (skin, details, page) in pages() {
        for scale in [1.0, 2.0] {
            let make = || {
                let mut e = BlueEditor::new(EditorHost::detached());
                let shared = Arc::new(Shared::default());
                e.bind(Some(shared.clone()));
                e.show(skin, details);
                let (width, height) = e.page().size();
                e.resize((width * 0.8) as u32, (height * 0.8) as u32);
                (e, shared)
            };
            let ((mut partial, shared_a), (mut full, shared_b)) = (make(), make());
            assert_eq!(partial.size(), if page == Page::Analogue { (1248, 360) } else { (1000, 720) });
            let spots: Vec<(f32, f32)> = partial.widget_params().iter().filter(|(_, p)| !p.is_empty()).map(|(r, _)| ((r.0 + r.2 * 0.5) * 0.8, (r.1 + r.3 * 0.5) * 0.8)).collect();
            for n in 0..30usize {
                let level = 0.05 + 0.6 * (n as f32 * 0.37).sin().abs();
                let telemetry = Telemetry {
                    input_peak: [level, level],
                    output_peak: [level * 1.7, level],
                    input_square: [level * level, 0.0],
                    output_square: [level * level * 0.5, 0.0],
                    comp_gr: 8.0 * level,
                    gate_gr: 40.0 * level,
                    deess_led: if n % 3 == 0 { 1.5 } else { 0.0 },
                    overload: i32::from(n % 7 == 0),
                    ..Telemetry::default()
                };
                let (px, py) = spots[(n * 7) % spots.len()];
                let param = (n * 5) % PARAMS.len();
                let value = if n % 2 == 0 { PARAMS[param].max } else { PARAMS[param].min };
                for (e, shared) in [(&mut partial, &shared_a), (&mut full, &shared_b)] {
                    shared.publish(&telemetry, 2, n % 4 == 0);
                    e.tick(1.0 / 30.0);
                    e.mouse_move(px, py);
                    // Not the input selector: it rebuilds the page, which repaints everything.
                    if param != ix::SOURCE {
                        e.param_changed(param, value);
                    }
                }
                full.damage.add_all();
                let (a, b) = (pixels_at(&mut partial, scale), pixels_at(&mut full, scale));
                let differing = a.iter().zip(&b).filter(|(x, y)| x != y).count();
                if differing > 0 {
                    let width = (partial.size().0 as f32 * scale) as usize;
                    let at: Vec<(usize, usize)> = a.iter().zip(&b).enumerate().filter(|(_, (x, y))| x != y).map(|(i, _)| (i % width, i / width)).collect();
                    let mut cells: Vec<(usize, usize)> = at.iter().map(|p| ((p.0 as f32 / scale / 0.8 / 20.0) as usize * 20, (p.1 as f32 / scale / 0.8 / 20.0) as usize * 20)).collect();
                    cells.sort_unstable();
                    cells.dedup();
                    eprintln!("in panel units, near {cells:?}; pointer at {:?}, parameter {}", (px / 0.8, py / 0.8), PARAMS[param].id);
                }
                assert_eq!(differing, 0, "{page:?} at {scale}x, step {n}: {differing} pixels differ");
            }
        }
    }
}

fn pixels_at(editor: &mut BlueEditor, scale: f32) -> Vec<u32> {
    editor.render(scale).pixels.to_vec()
}

/// The two skins have panels of different shapes. Changing skin asks the host for the other
/// page's window at the same scale, and the sizes the editor reports follow the page shown.
#[test]
fn each_skin_has_its_own_panel_size() {
    let asked: Rc<RefCell<Vec<(u32, u32)>>> = Rc::default();
    let sink = asked.clone();
    let host = EditorHost::detached().with_resize(move |w, h| {
        sink.borrow_mut().push((w, h));
        true
    });
    let mut e = BlueEditor::new(host);
    // The host answers a request with a resize, after the call that made it.
    let grant = |e: &mut BlueEditor| {
        let (w, h) = asked.borrow_mut().pop().expect("the editor asked for a size");
        e.resize(w, h);
    };
    let click = |e: &mut BlueEditor, param: usize| {
        let id = e.ctl.iter().flatten().copied().find(|id| e.controls.get(*id).unwrap().param == param).unwrap();
        let (x, y) = e.controls.get(id).unwrap().area.center();
        e.mouse_down(x, y, 1);
        e.mouse_up(x, y);
    };
    // It opens at full size; the rest of the test works at 0.8.
    assert_eq!((e.page(), e.size()), (Page::Analogue, (1560, 450)));
    e.resize(1248, 360);
    assert_eq!((e.page(), e.size()), (Page::Analogue, (1248, 360)));
    assert_eq!(e.size_range().aspect_ratio(), Some((1560, 450)));
    assert_eq!((e.size_range().clamp(1, 1), e.size_range().clamp(9999, 9999)), ((780, 225), (3120, 900)));
    click(&mut e, ix::MUTE);
    assert_eq!(e.values()[ix::MUTE], 1.0);

    // To the Digital skin, by its key: the same scale of the 1250 x 900 panel.
    let key = e.widgets.iter().find(|w| matches!(w.kind, Kind::SkinKey)).unwrap().rect;
    e.mouse_down((key.0 + 20.0) * 0.8, (key.1 + 8.0) * 0.8, 1);
    e.mouse_up((key.0 + 20.0) * 0.8, (key.1 + 8.0) * 0.8);
    assert_eq!(e.page(), Page::Overview);
    assert_eq!(e.size_range().aspect_ratio(), Some((1250, 900)));
    assert_eq!((e.size_range().clamp(1, 1), e.size_range().clamp(9999, 9999)), ((625, 450), (2500, 1800)));
    assert_eq!(*asked.borrow(), [(1000, 720)]);
    grant(&mut e);
    assert_eq!(e.size(), (1000, 720));
    e.show(1, true);
    assert!(asked.borrow().is_empty(), "the two Digital pages share a size");
    click(&mut e, ix::MUTE);
    assert_eq!(e.values()[ix::MUTE], 0.0);
    // Where Mute is on the analogue panel there is something else now.
    assert_eq!((e.render(1.0).width, e.render(1.0).height), (1000, 720));

    // A size the user drags to is one scale for both skins.
    e.resize(1250, 900);
    assert_eq!(e.settings().ui_scale, 1.0);
    let back = e.widgets.iter().find(|w| matches!(w.kind, Kind::Button { action: Action::Skin(0), .. })).unwrap().rect;
    e.mouse_down(back.0 + 20.0, back.1 + 8.0, 1);
    e.mouse_up(back.0 + 20.0, back.1 + 8.0);
    assert_eq!(e.page(), Page::Analogue);
    assert_eq!(e.size_range().aspect_ratio(), Some((1560, 450)));
    assert_eq!(*asked.borrow(), [(1560, 450)]);
    grant(&mut e);
    assert_eq!(e.size(), (1560, 450));
    click(&mut e, ix::COMPIN);
    assert_eq!(e.values()[ix::COMPIN], 1.0);
}

/// A host that does not give the size asked for: the page is drawn whole in the window it
/// has, as large as fits and centred, and clicks land on what is drawn.
#[test]
fn a_page_fits_a_window_of_another_shape() {
    let (mut e, _) = editor();
    e.show(1, true);
    // Still the analogue panel's 1560 x 450 window: the 1250 x 900 page at half size, 468 points in.
    assert_eq!((e.page(), e.size()), (Page::Controls, (1560, 450)));
    assert_eq!(e.fit(), (0.5, 467.0, 0.0));
    let frame = e.render(1.0);
    assert_eq!((frame.width, frame.height), (1560, 450));
    let pixels = frame.pixels.to_vec();
    let column = |x: usize| (0..450).map(|y| pixels[y * 1560 + x]).collect::<Vec<_>>();
    // Nothing but the background beside the page, the page inside.
    assert!(column(100).iter().all(|p| *p == pixels[0]) && column(1500).iter().all(|p| *p == pixels[0]));
    assert!(column(780).iter().any(|p| *p != pixels[0]));
    let (x, y, w, h) = centre_of(&e, ix::BYPASS);
    let at = (467.0 + (x + w * 0.5) * 0.5, (y + h * 0.5) * 0.5);
    e.mouse_down(at.0, at.1, 1);
    e.mouse_up(at.0, at.1);
    assert_eq!(e.values()[ix::BYPASS], 1.0);
    // A partial repaint there is a full one.
    let partial = pixels_at(&mut e, 1.0);
    e.damage.add_all();
    assert_eq!(partial, pixels_at(&mut e, 1.0));
    // Every tooltip and control is inside the page's part of the window.
    for tip in e.tooltips() {
        assert!(tip.rect.x >= 467.0 && tip.rect.x + tip.rect.w <= 467.0 + 625.0 + 1.0, "{:?}", tip.rect);
    }
}

#[test]
fn the_numeric_readouts_hold_a_value_long_enough_to_read() {
    use paint::READ_SECONDS;
    let (mut e, _) = editor();
    let shared = Arc::new(Shared::default());
    e.bind(Some(shared.clone()));
    let level = |peak: f32| Telemetry { input_peak: [peak, 0.0], output_peak: [peak, 0.0], comp_gr: 12.0 * peak, ..Telemetry::default() };
    let dt = 1.0 / 30.0;
    // The first reading shows at once.
    shared.publish(&level(0.5), 2, false);
    e.tick(dt);
    let first = e.meters.shown;
    assert!((first.input_db - 20.0 * 0.5f32.log10()).abs() < 1e-3 && first.comp_gr == 6.0);
    // Levels that change on every tick do not change the numbers until the hold is over...
    let mut ticks = 0;
    while (ticks + 1) as f32 * dt < READ_SECONDS - dt {
        shared.publish(&level(if ticks == 7 { 0.9 } else { 0.1 + 0.01 * (ticks % 5) as f32 }), 2, false);
        e.tick(dt);
        ticks += 1;
        assert_eq!(e.meters.shown, first, "changed after {ticks} ticks");
    }
    // ...and then they show the highest reading of that time, not the last one.
    for _ in 0..3 {
        shared.publish(&level(0.1), 2, false);
        e.tick(dt);
    }
    assert!((e.meters.shown.input_db - 20.0 * 0.9f32.log10()).abs() < 1e-3, "{}", e.meters.shown.input_db);
    assert!((e.meters.shown.comp_gr - 10.8).abs() < 1e-3);
    // The bars and the needle are not held: they follow every tick.
    assert!((e.meters.input_db - 20.0 * 0.1f32.log10()).abs() < 1e-3);
}
