#!/bin/bash
# SPDX-License-Identifier: MIT
# Build, test and package Blue Channel for macOS (universal arm64 + x86_64) as a bundle archive for
# the OpenGrid Plugin Manager. No PKG, DMG or standalone app is made.
#
#   installer/build_macos.sh                        unsigned archive (local testing)
#   SIGN=1 installer/build_macos.sh                 sign bundles, notarize the archive
#   SIGN=1 NOTARIZE=0 installer/build_macos.sh      sign locally, do not submit for notarization
#
# Identities default to the OpenGrid Developer ID used for earlier releases; override with
# BLUE_APP_ID / BLUE_INSTALLER_ID. Notarization uses the keychain profile in BLUE_NOTARY_PROFILE (default
# AC_NOTARY). Run SIGN=1 from your own Terminal: codesign and notarytool may need the keychain prompt.
# Optional: JUCE_SOURCE / CLAP_EXT_SOURCE point at local checkouts instead of downloading.
# AAX is built when BLUE_AAX_SDK is present or at ../aax-sdk-2-8-1.
set -euo pipefail
export COPYFILE_DISABLE=1
cd "$(dirname "$0")/.."
VERSION=$(tr -d '[:space:]' < VERSION)
BUILD=${BUILD:-build/release-macos}
OUT=${OUT:-dist/release-$VERSION/macos}
ART="$BUILD/BlueChannel_artefacts/Release"
APP_ID=${BLUE_APP_ID:-"Developer ID Application: Marcos Deida (ZQYU67JUBH)"}
NOTARY_PROFILE=${BLUE_NOTARY_PROFILE:-AC_NOTARY}
AAX_WCGUID=${BLUE_AAX_WCGUID:-"8692A2A0-CF31-11F1-898A-00505692C25A"}
PACE_WRAPTOOL=${BLUE_WRAPTOOL:-/Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool}
PACE_ACCOUNT=${BLUE_PACE_ACCOUNT:-mistamin}
AAX_SDK=${BLUE_AAX_SDK:-"$PWD/../aax-sdk-2-8-1"}

args=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DBLUECHANNEL_UNIVERSAL=ON -DBLUECHANNEL_DEV_EDITOR=OFF)
if [ "${BUILD_AAX:-1}" = "1" ] && [ -d "$AAX_SDK/Interfaces" ]; then
  args+=(-DBLUECHANNEL_BUILD_AAX=ON "-DBLUECHANNEL_AAX_SDK=$AAX_SDK")
else
  args+=(-DBLUECHANNEL_BUILD_AAX=OFF)
fi
if [ "${HQ:-1}" = "1" ]; then
  SIM="$PWD/build/deps/ngspice-universal"
  [ -f "$SIM/lib/libngspice.dylib" ] || tools/build_hq_simulator_macos.sh
  args+=(-DBLUECHANNEL_HQ=ON "-DBLUECHANNEL_NGSPICE_INCLUDE=$SIM/include"
         "-DBLUECHANNEL_NGSPICE_LIBRARY=$SIM/lib/libngspice.dylib" "-DBLUECHANNEL_HQ_NOTICES=$SIM/COPYING")
else
  args+=(-DBLUECHANNEL_HQ=OFF)
fi
[ -n "${JUCE_SOURCE:-}" ] && args+=("-DBLUECHANNEL_JUCE_SOURCE=$JUCE_SOURCE")
[ -n "${CLAP_EXT_SOURCE:-}" ] && args+=("-DBLUECHANNEL_CLAP_EXT_SOURCE=$CLAP_EXT_SOURCE")
cmake -S . -B "$BUILD" "${args[@]}"
cmake --build "$BUILD" --config Release
ctest --test-dir "$BUILD" --output-on-failure
if arch -x86_64 /usr/bin/true 2>/dev/null; then
  # The x86_64 slice under Rosetta: exercises the code path, not physical Intel performance.
  arch -x86_64 "$BUILD/BlueChannelModelTest"
fi
python3 tools/release_manifest.py "$BUILD"
case "$OUT" in dist/*|"$PWD"/dist/*) ;; *) echo "OUT must be inside this repository's dist folder" >&2; exit 1;; esac

rm -rf "$OUT"
RES="$OUT/resources"
mkdir -p "$RES/Licenses" "$RES/Manual/images"
python3 tools/collect_licenses.py "$BUILD" "$RES/Licenses/third-party" mac
cp LICENSE BINARY_LICENSE.txt THIRD_PARTY_LICENSES.md NGSPICE_SOURCE_NOTICE.txt "$RES/Licenses/"
cp docs/USER_MANUAL.md docs/HQ_MODE.md docs/PARAMETERS.md docs/RELEASE_1.0.0.md "$RES/Manual/"
cp docs/images/*.png "$RES/Manual/images/"
cp "$BUILD/release-manifest.json" "$RES/Manual/"

if [ "${SIGN:-0}" = "1" ]; then
  # Nested libraries first, then each bundle. AAX is PACE-signed after the Developer ID signature.
  for b in "$ART/VST3/Blue Channel.vst3" "$ART/AU/Blue Channel.component" "$ART/CLAP/Blue Channel.clap"; do
    find "$b/Contents" -type f -name '*.dylib' -print0 | while IFS= read -r -d '' f; do
      codesign --force --timestamp --options runtime --sign "$APP_ID" "$f"
    done
    codesign --force --deep --timestamp --options runtime --sign "$APP_ID" "$b"
    codesign --verify --deep --strict --verbose=2 "$b"
  done
  find "$ART/LV2/Blue Channel.lv2" -type f \( -name '*.dylib' -o -name '*.so' \) -print0 | while IFS= read -r -d '' f; do
    codesign --force --timestamp --options runtime --sign "$APP_ID" "$f"
    codesign --verify --strict --verbose=2 "$f"
  done
  if [ -d "$ART/AAX/Blue Channel.aaxplugin" ]; then
    codesign --force --deep --timestamp --options runtime --sign "$APP_ID" "$ART/AAX/Blue Channel.aaxplugin"
    [ -n "$AAX_WCGUID" ] && [ -n "$PACE_ACCOUNT" ] || { echo "BLUE_AAX_WCGUID and BLUE_PACE_ACCOUNT must be set for AAX" >&2; exit 1; }
    "$PACE_WRAPTOOL" sign --verbose --account "$PACE_ACCOUNT" --wcguid "$AAX_WCGUID" --signid "$APP_ID" \
      --in "$ART/AAX/Blue Channel.aaxplugin" --out "$ART/AAX/Blue Channel.aaxplugin" --autoinstall on
    "$PACE_WRAPTOOL" verify --verbose --in "$ART/AAX/Blue Channel.aaxplugin"
  fi
fi

MIN_OS=11.0 RESOURCES_DIR="$RES" tools/make_bundle_zip.sh "Blue Channel" "$VERSION" "$ART" "$OUT"
ZIP="$OUT/Blue-Channel-$VERSION-macos-bundles.zip"
if [ "${SIGN:-0}" = "1" ] && [ "${NOTARIZE:-1}" = "1" ]; then
  xcrun notarytool submit "$ZIP" --keychain-profile "$NOTARY_PROFILE" --wait
fi
(cd "$OUT" && shasum -a 256 "$(basename "$ZIP")" > "Blue-Channel-$VERSION-macos.sha256")
cat "$OUT/Blue-Channel-$VERSION-macos.sha256"
echo "done: $ZIP"
