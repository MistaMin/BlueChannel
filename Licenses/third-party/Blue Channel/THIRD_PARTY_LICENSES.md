# Third-party licenses

Blue Channel's own source code is MIT licensed (see `LICENSE`). Official complete binaries are
proprietary freeware under the publisher's commercial JUCE licence (see `BINARY_LICENSE.txt`), not a
relicensing of JUCE or other dependencies under MIT. End users need no JUCE developer licence to use
these official downloads. Independent builders must satisfy the JUCE licence applicable to their builds. Installers and archives include the license
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
| AAX SDK (Avid) | AAX format | Avid AAX Developer Agreement / SDK License |
| zlib, libpng, HarfBuzz, libwebp, lunasvg, SheenBidi and audio codecs (bundled with JUCE) | JUCE internals | their own licenses, included |

The AAX SDK is an external proprietary dependency under the Avid AAX Developer Agreement and is not committed to this repository. Official AAX binaries are digitally signed with PACE Eden for Pro Tools compatibility.

VST is a trademark of Steinberg Media Technologies GmbH. AAX and Pro Tools are trademarks of Avid Technology, Inc. Other format names are used only to describe compatibility.

## Circuit data

The stage netlists in `Source/Circuit/Netlists.h` are OpenGrid transcriptions of a reference circuit model,
with numerical component identifiers kept for traceability. Device behaviour uses generic published component data and
OpenGrid's own measurements of the reference model (`docs/measurements`). No third-party schematics,
manuals, product names or artwork are distributed.

Python is a development dependency and is not bundled. HQ-enabled releases include a dynamically loaded
shared ngspice 47 library and its complete notices. Our build disables OSDI, KLU, OpenMP and readline; it retains the default sparse solver, CIDER, the public-domain
XSPICE core (needed for numerical shunts) and the LGPL numeric-parameter code. Optional external
code-model modules, including the GPL table modules, are not shipped or loaded. The library's
BSD/MIT/LGPL permissions remain in force. Corresponding upstream source and the exact macOS build script
are included as a separate release source archive; see `NGSPICE_SOURCE_NOTICE.txt`.


The open-source licence for JUCE 9 is **AGPLv3**, not plain GPL. A commercial JUCE licence is the alternative.
See the [official licence](https://juce.com/legal/juce-9-licence/) and the exact framework licence copied
into each package. The publisher's commercial licence is not transferred with the MIT source.
No original third-party hardware manuals, schematic drawings, OCR extracts or research images are included.
