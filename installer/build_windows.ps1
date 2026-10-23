# SPDX-License-Identifier: MIT
# Build, test and package Blue Channel for Windows x64: VST3, CLAP, LV2 and Standalone.
#   powershell -ExecutionPolicy Bypass -File installer\build_windows.ps1
# Requires Visual Studio 2022 (C++), CMake 3.22+, Python 3 and Git. Inno Setup 6 (ISCC.exe on PATH) is
# optional: without it only the zip is produced. AAX is disabled and never packaged.
# Optional environment: JUCE_SOURCE, CLAP_EXT_SOURCE (local checkouts), SIGN_CERT_THUMBPRINT (Authenticode).
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$Version = (Get-Content VERSION -Raw).Trim()
$Build = if ($env:BUILD) { $env:BUILD } else { "build\release-windows" }
$Out = "dist\windows"
$Art = "$Build\BlueChannel_artefacts\Release"

if (-not (Test-Path "$Art\Standalone\Blue Channel.exe")) {
    $cfg = @("-S", ".", "-B", $Build, "-G", "Visual Studio 17 2022", "-A", "x64", "-DBLUECHANNEL_HQ=OFF", "-DBLUECHANNEL_BUILD_AAX=OFF")
    if ($env:JUCE_SOURCE) { $cfg += "-DBLUECHANNEL_JUCE_SOURCE=$($env:JUCE_SOURCE)" }
    if ($env:CLAP_EXT_SOURCE) { $cfg += "-DBLUECHANNEL_CLAP_EXT_SOURCE=$($env:CLAP_EXT_SOURCE)" }
    cmake @cfg
    cmake --build $Build --config Release --parallel
}
ctest --test-dir $Build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests failed" }

function CacheValue($name) {
    $line = Select-String -Path "$Build\CMakeCache.txt" -Pattern "^$name`:[^=]*=" | Select-Object -First 1
    if ($line) { return ($line.Line -split '=', 2)[1] }
    return $null
}
$JuceSrc = CacheValue "FETCHCONTENT_SOURCE_DIR_JUCE"
if (-not $JuceSrc) { $JuceSrc = CacheValue "BLUECHANNEL_JUCE_SOURCE" }
if (-not $JuceSrc) { $JuceSrc = "$Build\_deps\juce-src" }
$ClapExtSrc = CacheValue "FETCHCONTENT_SOURCE_DIR_CLAP-JUCE-EXTENSIONS"
if (-not $ClapExtSrc) { $ClapExtSrc = CacheValue "BLUECHANNEL_CLAP_EXT_SOURCE" }
if (-not $ClapExtSrc) { $ClapExtSrc = "$Build\_deps\clap-juce-extensions-src" }

function GitRev($p) {
    try { return (git -C $p rev-parse HEAD 2>$null).Trim() } catch { return "unknown" }
}
$srcFiles = @(Get-Item "CMakeLists.txt", "VERSION") + (Get-ChildItem -Path "Source" -Recurse -File -Include *.h,*.cpp | Sort-Object FullName)
$rootLen = (Resolve-Path ".").Path.Length + 1
$shaMap = [ordered]@{}
foreach ($f in $srcFiles) {
    $rel = $f.FullName.Substring($rootLen).Replace('\', '/')
    $shaMap[$rel] = (Get-FileHash $f.FullName -Algorithm SHA256).Hash.ToLower()
}
$gluiRev = (Get-Content "third_party\GoodLookinUI\UPSTREAM.json" -Raw | ConvertFrom-Json).revision
$manifestObj = [ordered]@{
    product = "Blue Channel"
    company = "OpenGrid"
    version = $Version
    source_commit = GitRev "."
    source_tree_clean = ((git status --porcelain | Out-String).Trim() -eq "")
    host_platform = "Windows AMD64"
    architectures = @("AMD64")
    minimum_macos = CacheValue "CMAKE_OSX_DEPLOYMENT_TARGET"
    configuration = "Release"
    distributed_formats = @("VST3", "AU (macOS)", "CLAP", "LV2", "Standalone")
    not_distributed = @("AAX")
    dependencies = [ordered]@{
        JUCE = GitRev $JuceSrc
        GoodLookinUI = $gluiRev
        "clap-juce-extensions" = GitRev $ClapExtSrc
        CLAP = GitRev "$ClapExtSrc\clap-libs\clap"
        "clap-helpers" = GitRev "$ClapExtSrc\clap-libs\clap-helpers"
    }
    compiler = (CacheValue "CMAKE_CXX_COMPILER_ID") + " " + (CacheValue "CMAKE_CXX_COMPILER_VERSION")
    hq_enabled = $false
    developer_editor = $false
    licensing = [ordered]@{
        project_source = "MIT"
        official_binaries = "Freeware under publisher commercial JUCE licence"
    }
    source_sha256 = $shaMap
}
$manifestObj | ConvertTo-Json -Depth 5 | Set-Content "$Build\release-manifest.json" -Encoding UTF8

foreach ($required in @("$Art\VST3\Blue Channel.vst3", "$Art\CLAP\Blue Channel.clap", "$Art\LV2\Blue Channel.lv2",
                        "$Art\Standalone\Blue Channel.exe")) {
    if (-not (Test-Path $required)) { throw "Missing build artifact: $required" }
}

if (Test-Path $Out) { Remove-Item -Recurse -Force $Out }
$Name = "BlueChannel-$Version-windows-x64"
$Stage = "$Out\$Name"
New-Item -ItemType Directory -Force -Path $Stage | Out-Null
Copy-Item -Recurse "$Art\VST3\Blue Channel.vst3", "$Art\CLAP\Blue Channel.clap", "$Art\LV2\Blue Channel.lv2" $Stage
Copy-Item "$Art\Standalone\Blue Channel.exe" $Stage

$LicenseRoot = "$Stage\Licenses\third-party"
foreach ($d in @("$LicenseRoot\Blue Channel", "$LicenseRoot\GoodLookinUI", "$LicenseRoot\JUCE", "$LicenseRoot\CLAP", "$LicenseRoot\VST3-SDK", "$LicenseRoot\LV2-SDK")) {
    New-Item -ItemType Directory -Force -Path $d | Out-Null
}
Copy-Item "LICENSE", "THIRD_PARTY_LICENSES.md", "BINARY_LICENSE.txt", "NGSPICE_SOURCE_NOTICE.txt" "$LicenseRoot\Blue Channel\"
Copy-Item "third_party\GoodLookinUI\LICENSE" "$LicenseRoot\GoodLookinUI\LICENSE"
Copy-Item "$JuceSrc\LICENSE.md" "$LicenseRoot\JUCE\LICENSE.md"
Copy-Item "$ClapExtSrc\LICENSE.md" "$LicenseRoot\CLAP\clap-juce-extensions-LICENSE.md"
Copy-Item "$ClapExtSrc\clap-libs\clap\LICENSE" "$LicenseRoot\CLAP\clap-LICENSE"
Copy-Item "$ClapExtSrc\clap-libs\clap-helpers\LICENSE" "$LicenseRoot\CLAP\clap-helpers-LICENSE"

$SearchRoots = @(
    "$JuceSrc\modules\juce_audio_formats\codecs",
    "$JuceSrc\modules\juce_core\zip\zlib",
    "$JuceSrc\modules\juce_graphics\drawables\lunasvg",
    "$JuceSrc\modules\juce_graphics\fonts\harfbuzz",
    "$JuceSrc\modules\juce_graphics\image_formats\libwebp",
    "$JuceSrc\modules\juce_graphics\image_formats\pnglib",
    "$JuceSrc\modules\juce_graphics\unicode\sheenbidi",
    "$JuceSrc\modules\juce_audio_processors_headless\format_types\VST3_SDK",
    "$JuceSrc\modules\juce_audio_processors_headless\format_types\LV2_SDK"
)
Get-ChildItem -Path $SearchRoots -Recurse -File -Include "LICENSE*", "COPYING*", "NOTICE*" | ForEach-Object {
    $p = $_.FullName
    if ($p -match [regex]::Escape("format_types\VST3_SDK\")) {
        $rel = $p -replace ".*format_types\\VST3_SDK\\", ""
        $dest = "$LicenseRoot\VST3-SDK\$rel"
    } elseif ($p -match [regex]::Escape("format_types\LV2_SDK\")) {
        $rel = $p -replace ".*format_types\\LV2_SDK\\", ""
        $dest = "$LicenseRoot\LV2-SDK\$rel"
    } elseif ($p -match [regex]::Escape("\modules\")) {
        $rel = $p -replace ".*\\modules\\", ""
        $dest = "$LicenseRoot\JUCE\modules\$rel"
    } else { return }
    New-Item -ItemType Directory -Force -Path (Split-Path $dest -Parent) | Out-Null
    Copy-Item $p $dest
}
if (Test-Path "$JuceSrc\modules\juce_graphics\image_formats\jpglib\README") {
    $jpgDest = "$LicenseRoot\JUCE\modules\juce_graphics\image_formats\jpglib\README"
    New-Item -ItemType Directory -Force -Path (Split-Path $jpgDest -Parent) | Out-Null
    Copy-Item "$JuceSrc\modules\juce_graphics\image_formats\jpglib\README" $jpgDest
}
Get-ChildItem -Recurse -File $LicenseRoot |
    ForEach-Object { $_.FullName.Substring((Resolve-Path $LicenseRoot).Path.Length + 1).Replace('\', '/') } |
    Sort-Object | Set-Content "$LicenseRoot\INDEX.txt"

Copy-Item LICENSE, BINARY_LICENSE.txt, THIRD_PARTY_LICENSES.md, NGSPICE_SOURCE_NOTICE.txt, docs\USER_MANUAL.md, docs\HQ_MODE.md, docs\PARAMETERS.md, docs\RELEASE_1.0.0.md, "$Build\release-manifest.json" $Stage

Copy-Item -Recurse docs\images "$Stage\images"

if ($env:SIGN_CERT_THUMBPRINT) {
    Get-ChildItem -Recurse $Stage -Include *.vst3, *.clap, *.dll, *.exe | Where-Object { -not $_.PSIsContainer } | ForEach-Object {
        signtool sign /sha1 $env:SIGN_CERT_THUMBPRINT /fd sha256 /tr http://timestamp.digicert.com /td sha256 $_.FullName
    }
}
if (Get-ChildItem -Recurse $Stage -Filter *.aaxplugin) { throw "AAX found in package" }

Compress-Archive -Path $Stage -DestinationPath "$Out\$Name.zip" -Force
if (Get-Command ISCC.exe -ErrorAction SilentlyContinue) {
    ISCC.exe /DMyAppVersion=$Version /DStageDir="$((Resolve-Path $Stage).Path)" installer\BlueChannel.iss
}
Get-ChildItem $Out -File | ForEach-Object { "{0}  {1}" -f (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower(), $_.Name } |
    Set-Content "$Out\$Name.sha256"
Get-Content "$Out\$Name.sha256"
