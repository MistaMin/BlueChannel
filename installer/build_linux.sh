#!/bin/bash
# SPDX-License-Identifier: MIT
# Build, test and package Blue Channel for Linux (x86_64 or aarch64): VST3, CLAP, LV2 and Standalone.
#   installer/build_linux.sh
# Build dependencies (Debian/Ubuntu): see docs/BUILDING.md (ALSA, X11, Freetype, fontconfig, GL, curl-free).
# Optional: JUCE_SOURCE / CLAP_EXT_SOURCE point at local checkouts instead of downloading.
set -euo pipefail
cd "$(dirname "$0")/.."
VERSION=$(tr -d '[:space:]' < VERSION)
ARCH=$(uname -m)
BUILD=${BUILD:-build/release-linux}
OUT=${OUT:-dist/linux}
ART="$BUILD/BlueChannel_artefacts/Release"

args=(-G Ninja -DCMAKE_BUILD_TYPE=Release)
[ -n "${JUCE_SOURCE:-}" ] && args+=("-DBLUECHANNEL_JUCE_SOURCE=$JUCE_SOURCE")
[ -n "${CLAP_EXT_SOURCE:-}" ] && args+=("-DBLUECHANNEL_CLAP_EXT_SOURCE=$CLAP_EXT_SOURCE")
cmake -S . -B "$BUILD" "${args[@]}"
cmake --build "$BUILD" --config Release
ctest --test-dir "$BUILD" --output-on-failure
python3 tools/release_manifest.py "$BUILD"

NAME="BlueChannel-$VERSION-linux-$ARCH"
rm -rf "$OUT"
mkdir -p "$OUT/$NAME"
cp -R "$ART/VST3/Blue Channel.vst3" "$ART/CLAP/Blue Channel.clap" "$ART/LV2/Blue Channel.lv2" "$OUT/$NAME/"
cp "$ART/Standalone/Blue Channel" "$OUT/$NAME/blue-channel"
python3 tools/collect_licenses.py "$BUILD" "$OUT/$NAME/Licenses" linux
cp LICENSE THIRD_PARTY_LICENSES.md docs/USER_MANUAL.md "$BUILD/release-manifest.json" "$OUT/$NAME/"
cat > "$OUT/$NAME/INSTALL.txt" <<TXT
Blue Channel $VERSION for Linux ($ARCH)

Per-user install:
  mkdir -p ~/.vst3 ~/.clap ~/.lv2
  cp -R "Blue Channel.vst3" ~/.vst3/
  cp -R "Blue Channel.clap" ~/.clap/
  cp -R "Blue Channel.lv2" ~/.lv2/
The standalone application is ./blue-channel.
TXT
if find "$OUT/$NAME" -iname '*.aaxplugin' | grep -q .; then echo "AAX found in package" >&2; exit 1; fi
(cd "$OUT" && tar -czf "$NAME.tar.gz" "$NAME" && sha256sum "$NAME.tar.gz" > "$NAME.sha256" && cat "$NAME.sha256")
