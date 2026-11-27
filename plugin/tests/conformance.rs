//! The port through the SDK's conformance kit: every sample rate and block
//! size, hostile input, parameter fuzz, state round trips, tail and layouts.
//!
//! Its allocation check counts Rust's allocator only: an allocation inside the
//! C++ engine would not be seen (SDK docs/CPP_BRIDGE.md says how to count those).

use blue_channel::BlueChannel;
use openplugin_testing::{conformance, Check, Options, Status};

openplugin_testing::install_allocator!();

#[test]
fn passes_the_conformance_kit() {
    // The standard: the quick run in a debug build, the whole kit in a release build.
    let options = if cfg!(debug_assertions) { Options::quick() } else { Options::default() };
    let report = conformance(BlueChannel::default, options);
    report.assert_pass();
    assert_eq!(report.status(Check::NoAlloc), Status::Pass, "{report}");
}
