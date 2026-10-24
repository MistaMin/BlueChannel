#!/bin/bash
# SPDX-License-Identifier: MIT
# Builds, signs, notarizes and staples the macOS DMG for Blue Channel.
# Includes the signed .pkg installer, documentation, and all license files.
set -euo pipefail
export COPYFILE_DISABLE=1

cd "$(dirname "$0")/.."
VERSION=$(tr -d '[:space:]' < VERSION)
OUT=${OUT:-"dist/release-$VERSION/macos"}
PKG="$OUT/BlueChannel-$VERSION-macos.pkg"
DMG="$OUT/BlueChannel-$VERSION-macos.dmg"
STAGE="$OUT/dmg-stage"
APP_ID=${BLUE_APP_ID:-"Developer ID Application: Marcos Deida (ZQYU67JUBH)"}
NOTARY_PROFILE=${BLUE_NOTARY_PROFILE:-AC_NOTARY}

if [ ! -f "$PKG" ]; then
  echo "Error: Package $PKG does not exist. Run installer/build_macos.sh first." >&2
  exit 1
fi

echo "==> Preparing DMG staging directory"
rm -rf "$STAGE" "$DMG"
mkdir -p "$STAGE"

# 1. Installer package
cp "$PKG" "$STAGE/"

# 2. Documentation
mkdir -p "$STAGE/Documentation"
cp docs/USER_MANUAL.md docs/HQ_MODE.md docs/PARAMETERS.md docs/RELEASE_1.0.0.md "$STAGE/Documentation/"
if [ -d docs/images ]; then
  mkdir -p "$STAGE/Documentation/images"
  cp docs/images/*.png "$STAGE/Documentation/images/"
fi

# 3. Licenses
mkdir -p "$STAGE/Licenses"
cp LICENSE BINARY_LICENSE.txt THIRD_PARTY_LICENSES.md NGSPICE_SOURCE_NOTICE.txt "$STAGE/Licenses/"
if [ -d "$OUT/stage/resources/Blue Channel/Licenses/third-party" ]; then
  ditto "$OUT/stage/resources/Blue Channel/Licenses/third-party" "$STAGE/Licenses/third-party"
elif [ -d Licenses/third-party ]; then
  ditto Licenses/third-party "$STAGE/Licenses/third-party"
fi

# 4. Root README / INSTALL instructions
cat > "$STAGE/INSTALL.txt" <<'EOF'
Blue Channel 1.0.0 — macOS Universal (Apple Silicon & Intel)
OpenGrid / Marcos Deida

INSTALLATION:
1. Double-click "BlueChannel-1.0.0-macos.pkg" to launch the installer.
2. Follow the standard macOS installer steps (Administrator privileges required).
3. The installer installs the following formats:
   - VST3:        /Library/Audio/Plug-Ins/VST3/Blue Channel.vst3
   - Audio Unit:  /Library/Audio/Plug-Ins/Components/Blue Channel.component
   - CLAP:        /Library/Audio/Plug-Ins/CLAP/Blue Channel.clap
   - LV2:         /Library/Audio/Plug-Ins/LV2/Blue Channel.lv2
   - AAX:         /Library/Application Support/Avid/Audio/Plug-Ins/Blue Channel.aaxplugin
   - Standalone:  /Applications/Blue Channel.app
4. Restart your DAW and rescan plugins.

DOCUMENTATION:
Refer to the "Documentation" folder on this disk image for the User Manual,
HQ Mode Guide, Parameter Reference, and Release Notes. Documentation and
licenses are also installed to:
/Library/Application Support/Blue Channel

LICENSING:
- Blue Channel source code is MIT licensed (see Licenses/LICENSE).
- Official binaries are distributed as freeware under the author's commercial JUCE licence (see Licenses/BINARY_LICENSE.txt).
- JUCE is open source (GPLv3 / AGPLv3) or commercial. See Licenses/THIRD_PARTY_LICENSES.md for third-party component licenses and attribution.
EOF

# Clean up unwanted metadata and attributes
xattr -cr "$STAGE" 2>/dev/null || true
find "$STAGE" -name '._*' -delete 2>/dev/null || true
find "$STAGE" -name '.DS_Store' -delete 2>/dev/null || true

echo "==> Creating disk image"
hdiutil create -volname "Blue Channel $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DMG"

echo "==> Signing DMG with Developer ID"
codesign --force --timestamp --sign "$APP_ID" "$DMG"
codesign --verify --verbose=2 "$DMG"

if [ "${NOTARIZE:-1}" = "1" ]; then
  echo "==> Submitting DMG for notarization"
  xcrun notarytool submit "$DMG" --keychain-profile "$NOTARY_PROFILE" --wait
  echo "==> Stapling notarization ticket to DMG"
  for retry in 1 2 3; do
    if xcrun stapler staple "$DMG"; then
      break
    fi
    echo "Staple attempt $retry failed; retrying in 3 seconds..."
    sleep 3
  done
  xcrun stapler validate "$DMG" || true
  echo "==> Assessing DMG with Gatekeeper"
  spctl --assess --type install -v "$DMG" || true
fi

echo "==> Updating SHA-256 checksums"
(cd "$OUT" && shasum -a 256 "$(basename "$PKG")" "BlueChannel-$VERSION-macos-bundles.zip" "$(basename "$DMG")" > "BlueChannel-$VERSION-macos.sha256")
cat "$OUT/BlueChannel-$VERSION-macos.sha256"

# Clean up temporary staging
rm -rf "$STAGE"

echo "==> Successfully built, signed, and notarized $DMG"
