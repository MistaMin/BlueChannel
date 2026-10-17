# Third-party licenses

Blue Channel's own source code is MIT licensed (see `LICENSE`). Installers and archives include the license
and notice texts below, copied from the exact dependency revisions of each build by
`tools/collect_licenses.py` (with an `INDEX.txt` listing every file).

| Component | Used for | License |
|---|---|---|
| JUCE 9.0.3 | plugin framework, user interface, plugin formats | AGPLv3 or the commercial JUCE licence. OpenGrid builds its released binaries under its own JUCE licence. Anyone building from source needs a JUCE licence that covers their use and distribution. |
| GoodLookinUI (`third_party/GoodLookinUI`) | knobs, keys, lamps, meters | MIT |
| CLAP, clap-helpers, clap-juce-extensions | CLAP format | MIT |
| VST3 SDK (bundled with JUCE) | VST3 format | MIT |
| LV2, serd, sord, sratom, lilv, zix (bundled with JUCE) | LV2 format | ISC |
| Audio Unit SDK (bundled with JUCE, macOS only) | Audio Unit format | Apache License 2.0 |
| zlib, libpng, HarfBuzz, libwebp, lunasvg, SheenBidi and audio codecs (bundled with JUCE) | JUCE internals | their own licenses, included |

The AAX SDK is not part of this repository and AAX builds are not distributed.

VST is a trademark of Steinberg Media Technologies GmbH. Other format names are used only to describe
compatibility.

## Circuit data

The stage netlists in `Source/Circuit/Netlists.h` are OpenGrid transcriptions of a reference circuit model,
with reference designators kept for traceability. Device behaviour uses generic published component data and
OpenGrid's own measurements of the reference model (`docs/measurements`). No third-party schematics,
manuals, product names or artwork are distributed.

Python and ngspice are used only by development tools; neither is bundled with the plugin.
