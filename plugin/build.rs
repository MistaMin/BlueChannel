//! Compiles the C++ engine (../Source/Model, header only, no JUCE) and its C
//! shim (cpp/bridge.cpp) for the Cargo target. `cc` passes the target's
//! architecture and MACOSX_DEPLOYMENT_TARGET, so a universal build is one
//! Cargo build per architecture joined by `lipo`.

use std::env;

fn main() {
    let manifest = env::var("CARGO_MANIFEST_DIR").unwrap_or_default();
    let out = env::var("OUT_DIR").unwrap_or_default();
    let mut build = cc::Build::new();
    build.cpp(true).std("c++20").include("cpp").include("../Source").file("cpp/bridge.cpp");
    // The offline component solver is the JUCE build's own source, with cpp/hq_shim standing in for
    // the few juce_core calls it makes. Feature `hq` compiles the solver in; it loads the shared
    // ngspice library at run time (a copy beside the plugin binary, else the path given here).
    build.include("cpp/hq_shim").file("../Source/Plugin/HQRenderer.cpp");
    println!("cargo:rerun-if-env-changed=BLUECHANNEL_NGSPICE_INCLUDE");
    println!("cargo:rerun-if-env-changed=BLUECHANNEL_NGSPICE_LIBRARY");
    if env::var_os("CARGO_FEATURE_HQ").is_some() {
        let include = env::var("BLUECHANNEL_NGSPICE_INCLUDE").unwrap_or_else(|_| "/opt/homebrew/include".into());
        let library = env::var("BLUECHANNEL_NGSPICE_LIBRARY").unwrap_or_else(|_| "/opt/homebrew/lib/libngspice.dylib".into());
        assert!(std::path::Path::new(&include).join("ngspice/sharedspice.h").is_file(), "feature hq: no ngspice/sharedspice.h in {include} (set BLUECHANNEL_NGSPICE_INCLUDE)");
        assert!(std::path::Path::new(&library).is_file(), "feature hq: no shared ngspice library at {library} (set BLUECHANNEL_NGSPICE_LIBRARY)");
        let name = std::path::Path::new(&library).file_name().unwrap().to_string_lossy().into_owned();
        build.include(&include);
        build.define("BLUECHANNEL_HQ", "1");
        // The solver is looked for beside the plugin binary first, then here. A packaged build
        // (scripts/build.sh sets BLUECHANNEL_HQ_PACKAGED) ships it in the bundle and keeps this
        // machine's path out of the binary.
        println!("cargo:rerun-if-env-changed=BLUECHANNEL_HQ_PACKAGED");
        let fallback = if env::var_os("BLUECHANNEL_HQ_PACKAGED").is_some() { name.clone() } else { library.clone() };
        build.define("BLUECHANNEL_HQ_LIBRARY", format!("\"{fallback}\"").as_str());
        build.define("BLUECHANNEL_HQ_LIBRARY_NAME", format!("\"{name}\"").as_str());
    } else {
        build.define("BLUECHANNEL_HQ", "0");
    }
    build.flag_if_supported("-fvisibility=hidden");
    // HQRenderer.cpp keeps a helper that only the solver build uses.
    build.flag_if_supported("-Wno-unused-function");
    if env::var_os("MACOSX_DEPLOYMENT_TARGET").is_none() && cfg!(target_os = "macos") {
        build.flag("-mmacosx-version-min=11.0"); // what rustc links with by default
    }
    // The model is numerical code: optimise it in debug builds too, or tests take minutes.
    if env::var("OPT_LEVEL").as_deref() == Ok("0") {
        build.opt_level(2);
    }
    // No build path in the shipped binary.
    let source = format!("{manifest}/../Source");
    for (from, to) in [(out.as_str(), "build"), (source.as_str(), "Source"), (manifest.as_str(), "plugin")] {
        if !from.is_empty() {
            build.flag_if_supported(format!("-ffile-prefix-map={from}={to}"));
        }
    }
    build.compile("bluechannel_engine");
    println!("cargo:rerun-if-changed=cpp");
    println!("cargo:rerun-if-changed=../Source/Plugin/HQRenderer.cpp");
    println!("cargo:rerun-if-changed=../Source/Plugin/HQRenderer.h");
    println!("cargo:rerun-if-changed=../Source/Model");
    println!("cargo:rerun-if-changed=../Source/Circuit");
    println!("cargo:rerun-if-changed=../Source/DSP");
    println!("cargo:rerun-if-changed=../Source/Dynamics");
}
