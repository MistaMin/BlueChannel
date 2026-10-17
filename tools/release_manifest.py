#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 OpenGrid
"""Write <build>/release-manifest.json: version, configuration, architectures, dependency revisions, compiler
and SHA-256 of every runtime source file and generated table. Usage: release_manifest.py <build dir>"""
import hashlib
import json
import platform
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else "build/release").resolve()
cache = {}
for line in (build / "CMakeCache.txt").read_text(errors="replace").splitlines():
    if line and not line.startswith(("#", "//")) and "=" in line and ":" in line.split("=", 1)[0]:
        k, v = line.split("=", 1)
        cache[k.split(":", 1)[0]] = v


def revision(path: Path):
    try:
        return subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip()
    except Exception:
        return "unknown"


def compiler():
    cxx = cache.get("CMAKE_CXX_COMPILER", "c++")
    try:
        return subprocess.check_output([cxx, "--version"], text=True, stderr=subprocess.STDOUT).splitlines()[0]
    except Exception:
        return cache.get("CMAKE_CXX_COMPILER_ID", "unknown") + " " + cache.get("CMAKE_CXX_COMPILER_VERSION", "")


juce = Path(cache.get("FETCHCONTENT_SOURCE_DIR_JUCE") or cache.get("BLUECHANNEL_JUCE_SOURCE") or build / "_deps/juce-src")
clap = Path(cache.get("FETCHCONTENT_SOURCE_DIR_CLAP-JUCE-EXTENSIONS") or cache.get("BLUECHANNEL_CLAP_EXT_SOURCE") or build / "_deps/clap-juce-extensions-src")
files = [root / "CMakeLists.txt", root / "VERSION"]
files += sorted((root / "Source").rglob("*.h")) + sorted((root / "Source").rglob("*.cpp"))
dirty = subprocess.run(["git", "-C", str(root), "status", "--porcelain"],
                       capture_output=True, text=True).stdout.strip()
manifest = {
    "product": "Blue Channel",
    "company": "OpenGrid",
    "version": (root / "VERSION").read_text().strip(),
    "source_commit": revision(root),
    "source_tree_clean": dirty == "",
    "host_platform": f"{platform.system()} {platform.machine()}",
    "architectures": cache.get("CMAKE_OSX_ARCHITECTURES", platform.machine()).split(";"),
    "minimum_macos": cache.get("CMAKE_OSX_DEPLOYMENT_TARGET"),
    "configuration": cache.get("CMAKE_BUILD_TYPE") or "Release",
    "distributed_formats": ["VST3", "AU (macOS)", "CLAP", "LV2", "Standalone"],
    "not_distributed": ["AAX"],
    "dependencies": {
        "JUCE": revision(juce),
        "GoodLookinUI": json.loads((root / "third_party/GoodLookinUI/UPSTREAM.json").read_text())["revision"],
        "clap-juce-extensions": revision(clap),
        "CLAP": revision(clap / "clap-libs/clap"),
        "clap-helpers": revision(clap / "clap-libs/clap-helpers"),
    },
    "compiler": compiler(),
    "hq_enabled": cache.get("BLUECHANNEL_HQ", "OFF").upper() in ("ON", "YES", "TRUE", "1"),
    "developer_editor": cache.get("BLUECHANNEL_DEV_EDITOR", "OFF").upper() in ("ON", "YES", "TRUE", "1"),
    "licensing": {"project_source": "MIT", "official_binaries": "Freeware under publisher commercial JUCE licence"},
    "source_sha256": {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
}
(build / "release-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
print("wrote", build / "release-manifest.json")
