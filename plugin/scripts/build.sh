#!/bin/sh
# Builds Blue Channel's macOS bundles (CLAP, LV2, VST3, Audio Unit, AAX, standalone app) with
# OpenPluginSDK's scripts/build_plugin.sh. Nothing is installed and nothing is signed beyond ad hoc.
#
#   scripts/build.sh [--hq] [--archs arm64|x86_64|universal] [--formats LIST] [--out DIR] [--work-dir DIR]
#
# --hq builds HQ Mode's offline component solver in and puts the shared ngspice library and its
# licence notices into every bundle (docs/HQ_MODE.md).
#
# Where things are found (set the variable, or keep the checkout in a Dependencies or SDKs folder
# beside or above this repository):
#   OPENPLUGIN_SDK   OpenPluginSDK checkout (GoodLookinDri beside it)
#   VST3_SDK, AU_SDK, AAX_SDK
#   BLUECHANNEL_NGSPICE_INCLUDE, BLUECHANNEL_NGSPICE_LIBRARY, BLUECHANNEL_HQ_NOTICES   for --hq;
#     default: ../build/deps/ngspice-universal, as built for the JUCE release
set -eu

here="$(cd "$(dirname "$0")/.." && pwd)"
root="$(cd "$here/.." && pwd)"

find_dir() { # env value, then candidate names
  if [ -n "$1" ] && [ -d "$1" ]; then echo "$1"; return 0; fi
  shift
  for name in "$@"; do
    for base in "$root/.." "$root/../.." "$root/../Dependencies" "$root/../../Dependencies" "$root/../SDKs" "$root/../../SDKs" /Volumes/*/Dependencies /Volumes/*/SDKs "$HOME/Dependencies" "$HOME/SDKs"; do
      [ -d "$base/$name" ] && { (cd "$base/$name" && pwd); return 0; }
    done
  done
  return 1
}

SDK="$(find_dir "${OPENPLUGIN_SDK:-}" OpenPluginSDK)" || { echo "OpenPluginSDK not found; set OPENPLUGIN_SDK" >&2; exit 1; }
VST3="$(find_dir "${VST3_SDK:-}" vst3sdk clap-wrapper/build/cpm/vst3sdk)" || { echo "VST3 SDK not found; set VST3_SDK" >&2; exit 1; }
AU="$(find_dir "${AU_SDK:-}" AudioUnitSDK)" || { echo "AudioUnitSDK not found; set AU_SDK" >&2; exit 1; }
AAX="$(find_dir "${AAX_SDK:-}" aax-sdk-2-8-1)" || { echo "AAX SDK not found; set AAX_SDK" >&2; exit 1; }
[ -f "$VST3/public.sdk/source/main/macmain.cpp" ] || { echo "$VST3 is not a complete VST3 SDK; set VST3_SDK" >&2; exit 1; }

hq=0
archs=universal
formats=clap,lv2,vst3,au,aax,standalone
out="$here/dist"
work="$here/target/bundles"
while [ $# -gt 0 ]; do
  case "$1" in
    --hq) hq=1; shift ;;
    --archs) archs="$2"; shift 2 ;;
    --formats) formats="$2"; shift 2 ;;
    --out) out="$2"; shift 2 ;;
    --work-dir) work="$2"; shift 2 ;;
    -h|--help) sed -n 2,16p "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

# Cargo takes the SDK from this checkout. The file holds machine paths and is not in Git.
if [ ! -f "$here/.cargo/config.toml" ]; then
  mkdir -p "$here/.cargo"
  cat > "$here/.cargo/config.toml" <<EOF
# Written by scripts/build.sh: where this machine keeps OpenPluginSDK.
[patch.crates-io]
openplugin = { path = "$SDK/crates/openplugin" }
openplugin-testing = { path = "$SDK/crates/openplugin-testing" }
goodlookindri-shell = { path = "$SDK/../GoodLookinDri/shell" }
EOF
fi

features=standalone
if [ "$hq" = 1 ]; then
  features=standalone,hq
  deps="$root/build/deps/ngspice-universal"
  BLUECHANNEL_NGSPICE_INCLUDE="${BLUECHANNEL_NGSPICE_INCLUDE:-$deps/include}"
  BLUECHANNEL_NGSPICE_LIBRARY="${BLUECHANNEL_NGSPICE_LIBRARY:-$deps/lib/libngspice.dylib}"
  BLUECHANNEL_HQ_NOTICES="${BLUECHANNEL_HQ_NOTICES:-$deps/COPYING}"
  BLUECHANNEL_HQ_PACKAGED=1
  export BLUECHANNEL_NGSPICE_INCLUDE BLUECHANNEL_NGSPICE_LIBRARY BLUECHANNEL_HQ_PACKAGED
  for f in "$BLUECHANNEL_NGSPICE_INCLUDE/ngspice/sharedspice.h" "$BLUECHANNEL_NGSPICE_LIBRARY" "$BLUECHANNEL_HQ_NOTICES"; do
    [ -f "$f" ] || { echo "--hq: $f is missing (see docs/HQ_MODE.md)" >&2; exit 1; }
  done
  if [ "$archs" = universal ]; then
    lipo "$BLUECHANNEL_NGSPICE_LIBRARY" -verify_arch arm64 x86_64 || { echo "--hq: a universal build needs an arm64+x86_64 ngspice" >&2; exit 1; }
  fi
fi

# The Audio Unit type and codes are given here because the shell build cannot ask a library
# that also holds C++ (its probe links as C). They are the ones `Plugin::au_ids` pins.
cd "$here"
"$SDK/scripts/build_plugin.sh" --manifest-path "$here/Cargo.toml" --name "Blue Channel" \
  --bundle-id com.opengrid.bluechannel --manufacturer OpenGrid --version "$(cat "$root/VERSION")" \
  --ttl-bin lv2_ttl --standalone-bin blue-channel-app --features "$features" --formats "$formats" \
  --editor --tail --aax-pages --archs "$archs" \
  --au-type aufx --au-subtype BlCh --au-manufacturer OpGr \
  --vst3-sdk "$VST3" --au-sdk "$AU" --aax-sdk "$AAX" --out "$out" --work-dir "$work"

# The standalone app's icon, the one the 1.0.0 app had.
app="$out/Blue Channel.app"
if [ -d "$app" ]; then
  mkdir -p "$app/Contents/Resources"
  cp "$root/Resources/BlueChannel.icns" "$app/Contents/Resources/BlueChannel.icns"
  /usr/libexec/PlistBuddy -c "Delete :CFBundleIconFile" "$app/Contents/Info.plist" 2>/dev/null || true
  /usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string BlueChannel" "$app/Contents/Info.plist"
  codesign --force --sign - "$app"
fi

if [ "$hq" = 1 ]; then
  # The solver beside each plugin binary, where it is looked for first, with its notices.
  library="$BLUECHANNEL_NGSPICE_LIBRARY"
  while [ -L "$library" ]; do
    target="$(readlink "$library")"
    case "$target" in /*) library="$target" ;; *) library="$(dirname "$library")/$target" ;; esac
  done
  name="$(basename "$BLUECHANNEL_NGSPICE_LIBRARY")"
  for bundle in "$out"/*.clap "$out"/*.vst3 "$out"/*.component "$out"/*.aaxplugin "$out"/*.app "$out"/*.lv2; do
    [ -d "$bundle" ] || continue
    if [ -d "$bundle/Contents/MacOS" ]; then
      binaries="$bundle/Contents/MacOS"; notices="$bundle/Contents/Resources/ThirdParty"
    else
      binaries="$bundle"; notices="$bundle/ThirdParty"
    fi
    mkdir -p "$notices"
    cp "$library" "$binaries/$name"
    cp "$BLUECHANNEL_HQ_NOTICES" "$notices/ngspice-COPYING.txt"
    codesign --force --sign - "$binaries/$name"
    if [ -d "$bundle/Contents" ]; then codesign --force --sign - "$bundle"; fi
    echo "HQ solver added: $bundle"
  done
fi
echo "bundles: $out"
