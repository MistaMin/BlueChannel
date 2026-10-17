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

$cfg = @("-S", ".", "-B", $Build, "-G", "Visual Studio 17 2022", "-A", "x64", "-DBLUECHANNEL_HQ=OFF", "-DBLUECHANNEL_BUILD_AAX=OFF")
if ($env:JUCE_SOURCE) { $cfg += "-DBLUECHANNEL_JUCE_SOURCE=$($env:JUCE_SOURCE)" }
if ($env:CLAP_EXT_SOURCE) { $cfg += "-DBLUECHANNEL_CLAP_EXT_SOURCE=$($env:CLAP_EXT_SOURCE)" }
cmake @cfg
cmake --build $Build --config Release --parallel
ctest --test-dir $Build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests failed" }
python tools\release_manifest.py $Build

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
python tools\collect_licenses.py $Build "$Stage\Licenses\third-party" win
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
