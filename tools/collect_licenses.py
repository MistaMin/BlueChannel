#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 OpenGrid
"""Collect the project license and the third-party notices for the code that the shipped binaries contain.

Usage: collect_licenses.py <build dir> <destination dir> <mac|win|linux>

Reads the JUCE and clap-juce-extensions locations from the build's CMakeCache.txt (falling back to the
FetchContent default paths), copies the license texts from those exact revisions, and writes INDEX.txt.
The Audio Unit SDK notices are included on macOS only. The AAX SDK is never included: AAX builds are not
distributed (docs/RELEASE.md).
"""
import shutil
import sys
from pathlib import Path


def cache(build: Path):
    values = {}
    for line in (build / "CMakeCache.txt").read_text(errors="replace").splitlines():
        if line and not line.startswith(("#", "//")) and "=" in line and ":" in line.split("=", 1)[0]:
            k, v = line.split("=", 1)
            values[k.split(":", 1)[0]] = v
    return values


def main():
    root = Path(__file__).resolve().parents[1]
    build, dest, platform = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve(), sys.argv[3]
    c = cache(build)
    juce = Path(c.get("FETCHCONTENT_SOURCE_DIR_JUCE") or build / "_deps/juce-src")
    clap = Path(c.get("FETCHCONTENT_SOURCE_DIR_CLAP-JUCE-EXTENSIONS") or build / "_deps/clap-juce-extensions-src")
    if dest.exists():
        shutil.rmtree(dest)

    def put(src: Path, rel: str):
        out = dest / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, out)

    put(root / "LICENSE", "Blue Channel/LICENSE")
    put(root / "THIRD_PARTY_LICENSES.md", "Blue Channel/THIRD_PARTY_LICENSES.md")
    put(root / "third_party/GoodLookinUI/LICENSE", "GoodLookinUI/LICENSE")
    put(juce / "LICENSE.md", "JUCE/LICENSE.md")
    put(clap / "LICENSE.md", "CLAP/clap-juce-extensions-LICENSE.md")
    put(clap / "clap-libs/clap/LICENSE", "CLAP/clap-LICENSE")
    put(clap / "clap-libs/clap-helpers/LICENSE", "CLAP/clap-helpers-LICENSE")

    mods = juce / "modules"
    roots = [
        mods / "juce_audio_formats/codecs",
        mods / "juce_core/zip/zlib",
        mods / "juce_graphics/drawables/lunasvg",
        mods / "juce_graphics/fonts/harfbuzz",
        mods / "juce_graphics/image_formats/libwebp",
        mods / "juce_graphics/image_formats/pnglib",
        mods / "juce_graphics/unicode/sheenbidi",
        mods / "juce_audio_processors_headless/format_types/VST3_SDK",
        mods / "juce_audio_processors_headless/format_types/LV2_SDK",
    ]
    if platform == "mac":
        roots.append(mods / "juce_audio_plugin_client/AU/AudioUnitSDK")
    for base in roots:
        if not base.exists():
            continue
        for f in base.rglob("*"):
            name = f.name.upper()
            if not f.is_file() or not name.startswith(("LICENSE", "COPYING", "NOTICE")):
                continue
            s = f.as_posix()
            if "/VST3_SDK/" in s:
                rel = "VST3-SDK/" + s.split("/VST3_SDK/", 1)[1]
            elif "/LV2_SDK/" in s:
                rel = "LV2-SDK/" + s.split("/LV2_SDK/", 1)[1]
            elif "/AudioUnitSDK/" in s:
                rel = "AudioUnit-SDK/" + s.split("/AudioUnitSDK/", 1)[1]
            else:
                rel = "JUCE/modules/" + s.split("/modules/", 1)[1]
            put(f, rel)
    files = sorted(p.relative_to(dest).as_posix() for p in dest.rglob("*") if p.is_file())
    (dest / "INDEX.txt").write_text("\n".join(files) + "\n")
    print(f"collected {len(files)} license files into {dest}")


if __name__ == "__main__":
    main()
