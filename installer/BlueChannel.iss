; SPDX-License-Identifier: MIT
; Blue Channel Windows installer (Inno Setup 6). Built by installer\build_windows.ps1, which stages the files
; and passes /DMyAppVersion and /DStageDir.
#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#ifndef StageDir
  #error StageDir must be defined
#endif
#define AppName "Blue Channel"
#define AppPublisher "OpenGrid"

[Setup]
AppId={{6F1E3C2A-3B7D-4C55-9E21-0B9C5A7D2F41}
AppName={#AppName}
AppVersion={#MyAppVersion}
AppVerName={#AppName} {#MyAppVersion}
AppPublisher={#AppPublisher}
VersionInfoVersion={#MyAppVersion}
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DisableDirPage=yes
OutputDir={#StageDir}\..
OutputBaseFilename=BlueChannel-{#MyAppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
LicenseFile={#StageDir}\LICENSE
UninstallDisplayName={#AppName}

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: full custom
Name: "clap"; Description: "CLAP plug-in"; Types: full custom
Name: "lv2"; Description: "LV2 plug-in"; Types: full custom
Name: "app"; Description: "Standalone application"; Types: full custom
Name: "docs"; Description: "User guide and license texts"; Types: full custom; Flags: fixed

[Files]
Source: "{#StageDir}\Blue Channel.vst3\*"; DestDir: "{commoncf64}\VST3\Blue Channel.vst3"; Components: vst3; Flags: recursesubdirs ignoreversion
Source: "{#StageDir}\Blue Channel.clap"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion
Source: "{#StageDir}\Blue Channel.lv2\*"; DestDir: "{commoncf64}\LV2\Blue Channel.lv2"; Components: lv2; Flags: recursesubdirs ignoreversion
Source: "{#StageDir}\Blue Channel.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "{#StageDir}\Licenses\*"; DestDir: "{app}\Licenses"; Components: docs; Flags: recursesubdirs ignoreversion
Source: "{#StageDir}\USER_MANUAL.md"; DestDir: "{app}"; Components: docs; Flags: ignoreversion
Source: "{#StageDir}\LICENSE"; DestDir: "{app}"; Components: docs; Flags: ignoreversion
Source: "{#StageDir}\THIRD_PARTY_LICENSES.md"; DestDir: "{app}"; Components: docs; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\Blue Channel.exe"; Components: app
