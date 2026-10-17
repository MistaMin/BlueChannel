#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 OpenGrid
# Reproducible universal shared simulator. No project research inputs are used.
set -euo pipefail
cd "$(dirname "$0")/.."
version=47
sha=894e649651f1838a14095e5a5439e7d3aa63e87ede14d283173fda4fcdef675f
root="$PWD/build/deps"
prefix="$root/ngspice-universal"
mkdir -p "$root"
archive="$root/ngspice-$version.tar.gz"
if [ ! -f "$archive" ]; then
  curl -fL --retry 2 "https://downloads.sourceforge.net/project/ngspice/ng-spice-rework/$version/ngspice-$version.tar.gz" -o "$archive"
fi
actual=$(shasum -a 256 "$archive" | cut -d ' ' -f 1)
[ "$actual" = "$sha" ] || { echo "Simulator source checksum mismatch" >&2; exit 1; }
[ -d "$root/ngspice-$version" ] || tar -xzf "$archive" -C "$root"
# Autotools rejects source directories containing spaces; use a private temporary build directory.
sim_work=$(mktemp -d /tmp/bluechannel-ngspice.XXXXXX)
trap 'rm -rf "$sim_work"' EXIT
tar -xzf "$archive" -C "$sim_work"
mkdir "$sim_work/compile"
cd "$sim_work/compile"
[ ! -d /opt/homebrew/opt/bison/bin ] || export PATH="/opt/homebrew/opt/bison/bin:$PATH"
[ ! -d /opt/homebrew/opt/flex/bin ] || export PATH="/opt/homebrew/opt/flex/bin:$PATH"
export MACOSX_DEPLOYMENT_TARGET=11.0
export CFLAGS="-O2 -arch arm64 -arch x86_64 -mmacosx-version-min=11.0"
export CXXFLAGS="$CFLAGS"
export LDFLAGS="-arch arm64 -arch x86_64 -mmacosx-version-min=11.0"
"$sim_work/ngspice-$version/configure" --prefix="$sim_work/install" --with-ngshared --enable-cider \
  --disable-openmp --disable-klu --disable-osdi --with-readline=no --without-x
# Build only library prerequisites. Optional external code-model modules are neither built nor installed.
for part in include/ngspice include/cppduals/duals misc maths frontend spicelib ciderlib \
            xspice/mif xspice/cm xspice/enh xspice/evt xspice/idn; do
  make -C "src/$part" -j"${JOBS:-8}"
done
make -C src -j"${JOBS:-8}" libngspice.la
mkdir -p "$prefix/lib" "$prefix/include/ngspice"
ditto "$sim_work/compile/src/.libs/libngspice.0.dylib" "$prefix/lib/libngspice.dylib"
cp "$sim_work/ngspice-$version/src/include/ngspice/sharedspice.h" "$prefix/include/ngspice/sharedspice.h"
cp "$sim_work/ngspice-$version/COPYING" "$prefix/COPYING"
install_name_tool -id @rpath/libngspice.0.dylib "$prefix/lib/libngspice.dylib"
codesign --force --sign - "$prefix/lib/libngspice.dylib"
lipo "$prefix/lib/libngspice.dylib" -verify_arch arm64 x86_64
otool -L "$prefix/lib/libngspice.dylib"
echo "Universal simulator installed in $prefix"
