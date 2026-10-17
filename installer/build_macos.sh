#!/bin/bash
# SPDX-License-Identifier: MIT
# Build, test and package Blue Channel for macOS (universal arm64 + x86_64).
#
#   installer/build_macos.sh                        unsigned package and zip (local testing)
#   SIGN=1 installer/build_macos.sh                 sign bundles and package, notarize and staple
#   SIGN=1 NOTARIZE=0 installer/build_macos.sh      sign locally, do not submit for notarization
#
# Identities default to the OpenGrid Developer ID used for earlier releases; override with
# BLUE_APP_ID / BLUE_INSTALLER_ID. Notarization uses the keychain profile in BLUE_NOTARY_PROFILE (default
# AC_NOTARY). Run SIGN=1 from your own Terminal: codesign and notarytool may need the keychain prompt.
# Optional: JUCE_SOURCE / CLAP_EXT_SOURCE point at local checkouts instead of downloading.
# AAX is disabled and never packaged.
set -euo pipefail
export COPYFILE_DISABLE=1
cd "$(dirname "$0")/.."
VERSION=$(tr -d '[:space:]' < VERSION)
BUILD=${BUILD:-build/release-macos}
OUT=${OUT:-dist/release-$VERSION/macos}
ART="$BUILD/BlueChannel_artefacts/Release"
APP_ID=${BLUE_APP_ID:-"Developer ID Application: Marcos Deida (ZQYU67JUBH)"}
INST_ID=${BLUE_INSTALLER_ID:-"Developer ID Installer: Marcos Deida (ZQYU67JUBH)"}
NOTARY_PROFILE=${BLUE_NOTARY_PROFILE:-AC_NOTARY}

args=(-G Ninja -DCMAKE_BUILD_TYPE=Release -DBLUECHANNEL_UNIVERSAL=ON -DBLUECHANNEL_DEV_EDITOR=OFF -DBLUECHANNEL_BUILD_AAX=OFF)
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
mkdir -p "$OUT/stage" "$OUT/pkgs"
RES="$OUT/stage/resources/Blue Channel"
mkdir -p "$RES/Manual"
python3 tools/collect_licenses.py "$BUILD" "$RES/Licenses/third-party" mac
cp LICENSE BINARY_LICENSE.txt THIRD_PARTY_LICENSES.md NGSPICE_SOURCE_NOTICE.txt "$RES/"
cp LICENSE BINARY_LICENSE.txt THIRD_PARTY_LICENSES.md NGSPICE_SOURCE_NOTICE.txt "$RES/Licenses/"
cp docs/USER_MANUAL.md docs/HQ_MODE.md docs/PARAMETERS.md docs/RELEASE_1.0.0.md "$RES/Manual/"
mkdir -p "$RES/Manual/images"
cp docs/images/*.png "$RES/Manual/images/"
cp "$BUILD/release-manifest.json" "$RES/Manual/"

declare -a BUNDLES=(
  "VST3/Blue Channel.vst3|/Library/Audio/Plug-Ins/VST3|vst3"
  "AU/Blue Channel.component|/Library/Audio/Plug-Ins/Components|au"
  "CLAP/Blue Channel.clap|/Library/Audio/Plug-Ins/CLAP|clap"
  "LV2/Blue Channel.lv2|/Library/Audio/Plug-Ins/LV2|lv2"
  "Standalone/Blue Channel.app|/Applications|app"
)
for entry in "${BUNDLES[@]}"; do
  IFS='|' read -r rel dest id <<<"$entry"
  name=$(basename "$rel")
  mkdir -p "$OUT/stage/$id"
  # ditto without extended attributes or resource forks: keeps AppleDouble entries out of the payload.
  ditto --norsrc --noextattr --noacl "$ART/$rel" "$OUT/stage/$id/$name"
  xattr -cr "$OUT/stage/$id/$name" 2>/dev/null || true
  find "$OUT/stage/$id" -name '._*' -delete
  if [ "$id" = "lv2" ]; then
    LICENSE_DEST="$OUT/stage/$id/$name/Licenses"
  else
    LICENSE_DEST="$OUT/stage/$id/$name/Contents/Resources/Licenses"
  fi
  ditto "$RES/Licenses" "$LICENSE_DEST"
  lipo -info "$(find "$OUT/stage/$id/$name" -type f -perm -111 \( -path '*/MacOS/*' -o -name '*.so' \) | head -n 1)"
  if [ "${SIGN:-0}" = "1" ]; then
    if [ "$id" = "lv2" ]; then
      find "$OUT/stage/$id/$name" -type f \( -name '*.dylib' -o -name '*.so' \) -print0 | while IFS= read -r -d '' f; do
        codesign --force --timestamp --options runtime --sign "$APP_ID" "$f"
        codesign --verify --strict --verbose=2 "$f"
      done
    else
      find "$OUT/stage/$id/$name/Contents" -type f -name '*.dylib' -print0 | while IFS= read -r -d '' f; do
        codesign --force --timestamp --options runtime --sign "$APP_ID" "$f"
      done
      codesign --force --deep --timestamp --options runtime --sign "$APP_ID" "$OUT/stage/$id/$name"
      codesign --verify --deep --strict --verbose=2 "$OUT/stage/$id/$name"
    fi
  fi
  pkgbuild --root "$OUT/stage/$id" --install-location "$dest" --identifier "com.opengrid.bluechannel.$id" \
           --version "$VERSION" "$OUT/pkgs/BlueChannel-$id.pkg" >/dev/null
done
pkgbuild --root "$OUT/stage/resources" --install-location "/Library/Application Support" \
         --identifier "com.opengrid.bluechannel.resources" --version "$VERSION" "$OUT/pkgs/BlueChannel-resources.pkg" >/dev/null

mkdir -p "$OUT/installer-resources"
python3 - "$OUT/installer-resources/license.html" <<'PYHTML'
import html,sys
from pathlib import Path
body='<h1>Blue Channel — licence terms</h1><p>Project source: MIT. Official binaries: freeware, built under the publisher’s commercial JUCE licence. JUCE 9 alternatively offers AGPLv3 terms for independent builders. Component licence rights remain intact.</p>'
for name,title in [('BINARY_LICENSE.txt','Official binary freeware licence'),('LICENSE','Project source licence')]:
    body+='<h2>'+title+'</h2><pre>'+html.escape(Path(name).read_text())+'</pre>'
Path(sys.argv[1]).write_text('<!doctype html><html><meta charset="utf-8"><style>body{font:13px -apple-system,sans-serif;color:#202737;padding:16px}pre{font:12px -apple-system,sans-serif;white-space:pre-wrap}</style>'+body+'</html>')
PYHTML

cat > "$OUT/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
  <title>Blue Channel $VERSION</title>
  <license file="license.html" mime-type="text/html"/>
  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <choices-outline>
    <line choice="vst3"/><line choice="au"/><line choice="clap"/><line choice="lv2"/><line choice="app"/><line choice="resources"/>
  </choices-outline>
  <choice id="vst3" title="VST3"><pkg-ref id="com.opengrid.bluechannel.vst3"/></choice>
  <choice id="au" title="Audio Unit"><pkg-ref id="com.opengrid.bluechannel.au"/></choice>
  <choice id="clap" title="CLAP"><pkg-ref id="com.opengrid.bluechannel.clap"/></choice>
  <choice id="lv2" title="LV2"><pkg-ref id="com.opengrid.bluechannel.lv2"/></choice>
  <choice id="app" title="Standalone application"><pkg-ref id="com.opengrid.bluechannel.app"/></choice>
  <choice id="resources" title="User guide and license texts" selected="true" visible="false"><pkg-ref id="com.opengrid.bluechannel.resources"/></choice>
  <pkg-ref id="com.opengrid.bluechannel.vst3">BlueChannel-vst3.pkg</pkg-ref>
  <pkg-ref id="com.opengrid.bluechannel.au">BlueChannel-au.pkg</pkg-ref>
  <pkg-ref id="com.opengrid.bluechannel.clap">BlueChannel-clap.pkg</pkg-ref>
  <pkg-ref id="com.opengrid.bluechannel.lv2">BlueChannel-lv2.pkg</pkg-ref>
  <pkg-ref id="com.opengrid.bluechannel.app">BlueChannel-app.pkg</pkg-ref>
  <pkg-ref id="com.opengrid.bluechannel.resources">BlueChannel-resources.pkg</pkg-ref>
</installer-gui-script>
XML
PKG="$OUT/BlueChannel-$VERSION-macos-unsigned.pkg"
productbuild --distribution "$OUT/distribution.xml" --package-path "$OUT/pkgs" --resources "$OUT/installer-resources" "$PKG" >/dev/null
FINAL="$PKG"
if [ "${SIGN:-0}" = "1" ]; then
  FINAL="$OUT/BlueChannel-$VERSION-macos.pkg"
  productsign --sign "$INST_ID" --timestamp "$PKG" "$FINAL"
  pkgutil --check-signature "$FINAL"
  rm -f "$PKG"
  if [ "${NOTARIZE:-1}" = "1" ]; then
    xcrun notarytool submit "$FINAL" --keychain-profile "$NOTARY_PROFILE" --wait
    xcrun stapler staple "$FINAL"
    xcrun stapler validate "$FINAL"
    spctl --assess --type install --verbose=2 "$FINAL"
  fi
fi

# Portable zip of the bundles (for users who install by hand).
ZIP="$OUT/BlueChannel-$VERSION-macos-bundles.zip"
(cd "$OUT/stage" && zip -qry "../$(basename "$ZIP")" vst3 au clap lv2 app resources)

# Distribution checks: no AAX, and no real AppleDouble files in the staged trees. (pkgbuild records extended
# attributes such as com.apple.provenance as ._ entries in its payload; Installer restores those as
# attributes, not files, so they are not checked here.)
if find "$OUT/stage" -name '._*' | grep -q .; then echo "AppleDouble files in the stage" >&2; exit 1; fi
if pkgutil --payload-files "$FINAL" 2>/dev/null | grep -qi '\.aaxplugin'; then echo "AAX found in package" >&2; exit 1; fi
if unzip -l "$ZIP" | grep -qi '\.aaxplugin'; then echo "AAX found in zip" >&2; exit 1; fi

# Checksums last: stapling changes bytes.
(cd "$OUT" && shasum -a 256 "$(basename "$FINAL")" "$(basename "$ZIP")" > "BlueChannel-$VERSION-macos.sha256")
cat "$OUT/BlueChannel-$VERSION-macos.sha256"
echo "done: $FINAL"
