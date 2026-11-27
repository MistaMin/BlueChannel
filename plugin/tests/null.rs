//! The null test: the original "Blue Channel" and this port render the same input, and
//! `openplugin-port null` compares them (docs/PORTING.md, "The null test").
//!
//! `OPENPLUGIN_PORT` names the `openplugin-port` executable (otherwise the one on
//! `PATH`); `OPENPLUGIN_PORT_ORIGINAL` names the original plugin (otherwise the
//! installed VST3).
//!
//! The JUCE build shows hosts every parameter as 0..1 and the port shows plain
//! units, so the two only take the same automation value for switches: the
//! automation below turns sections on and off. Continuous controls are compared
//! at their defaults.

use std::path::{Path, PathBuf};
use std::process::Command;

/// The original is the 1.0.0 JUCE release's VST3 (its CLAP build crashes in the tool's host).
const ORIGINAL_FORMAT: &str = "vst3";
/// This crate's library, as Cargo names it.
const LIB: &str = "blue_channel";
/// Built-in signals, each rendered and compared (`sweep`, `noise`, `impulses`, `burst`, `silence`, `:seconds`).
const INPUTS: &[&str] = &["sweep:1", "noise:1", "impulses:1"];
/// Largest sample difference allowed.
/// 1.0.0 and 1.1.0 agree to 1e-6. Since the mic transformer's core current is injected without its
/// content at half the sample rate (Source/Model/Stages.h: 1.0.0 overflows on a loud tone below
/// 100 Hz), the largest difference measured is 3.7e-6.
const TOLERANCE: &str = "1e-5";
/// Parameter moves for both plugins, as `frame:param=value`; one set per run.
/// Every frame is a multiple of the 512-frame block: the JUCE build applies a
/// change from the start of its block, the port at the frame itself.
const AUTOMATION: &[&str] = &[
    "",
    "0:Comp In=1, 0:Gate In=1, 0:De-Ess In=1, 0:Soft Limiter=1, 12288:Phase=1, 24064:Mic Air=1, 30208:Comp In=0",
    "0:Comp In=1, 0:Blend=1, 0:Auto Release=1, 0:Expand=1, 0:Gate In=1, 0:Fast Attack=1, 9216:Stereo Link=0, 19968:Mute=1, 26112:Mute=0, 35840:Bypass=1",
];

/// Build this crate's library and return the file Cargo reports for it (Cargo decides the
/// target folder and its layout, from CARGO_TARGET_DIR or its configuration).
fn port() -> PathBuf {
    let cargo = std::env::var("CARGO").unwrap_or_else(|_| "cargo".into());
    let out = Command::new(cargo)
        .args(["build", "--lib", "--message-format=json", "--manifest-path"])
        .arg(Path::new(env!("CARGO_MANIFEST_DIR")).join("Cargo.toml"))
        .output()
        .expect("run cargo");
    assert!(out.status.success(), "building the port failed:\n{}", String::from_utf8_lossy(&out.stderr));
    let name = format!("{}{LIB}{}", std::env::consts::DLL_PREFIX, std::env::consts::DLL_SUFFIX);
    let messages = String::from_utf8_lossy(&out.stdout);
    let lib = messages
        .lines()
        .filter(|l| l.contains("\"reason\":\"compiler-artifact\""))
        .flat_map(json_strings)
        .map(PathBuf::from)
        .find(|p| p.file_name().is_some_and(|n| n.to_string_lossy() == name))
        .unwrap_or_else(|| panic!("cargo did not report {name}"));
    assert!(lib.is_file(), "{} was not built", lib.display());
    lib
}

/// Every string value in one line of Cargo's JSON, unescaped (enough for paths).
fn json_strings(line: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut chars = line.chars();
    while let Some(c) = chars.next() {
        if c != '"' {
            continue;
        }
        let mut text = String::new();
        while let Some(c) = chars.next() {
            match c {
                '"' => break,
                '\\' => text.push(chars.next().unwrap_or('\\')),
                c => text.push(c),
            }
        }
        out.push(text);
    }
    out
}

#[test]
fn the_port_nulls_against_the_original() {
    // Both are files of this machine, so a checkout without them has nothing to compare with.
    let (Some(tool), Some(original)) = (std::env::var_os("OPENPLUGIN_PORT"), std::env::var_os("OPENPLUGIN_PORT_ORIGINAL")) else {
        eprintln!("skipped: set OPENPLUGIN_PORT (the openplugin-port tool) and OPENPLUGIN_PORT_ORIGINAL (the 1.0.0 VST3)");
        return;
    };
    let (tool, original, port) = (PathBuf::from(tool), PathBuf::from(original), port());
    assert!(original.exists(), "the original plugin {} is not there", original.display());
    for automation in AUTOMATION {
        for input in INPUTS {
            let mut null = Command::new(&tool);
            null.arg("null").arg(&original).arg(&port);
            null.args(["--format", ORIGINAL_FORMAT, "--port-format", "clap", "--input", input, "--tolerance", TOLERANCE]);
            if !automation.is_empty() {
                null.args(["--automation", automation]);
            }
            let out = null.output().unwrap_or_else(|e| panic!("cannot run {}: {e} (set OPENPLUGIN_PORT)", tool.display()));
            let report = String::from_utf8_lossy(&out.stdout);
            println!("input {input}, automation {automation:?}:\n{report}");
            assert!(out.status.success(), "input {input}, automation {automation:?}: the port does not null against the original ({})\n{report}{}", out.status, String::from_utf8_lossy(&out.stderr));
        }
    }
}
