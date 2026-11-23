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
    let report = conformance(BlueChannel::default, Options::quick());
    report.assert_pass();
    assert_eq!(report.status(Check::NoAlloc), Status::Pass, "{report}");
}
