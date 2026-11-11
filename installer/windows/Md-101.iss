; ==============================================================================
; Md-101 — Inno Setup 6 Installer Script (Windows x64)
; Periodic Table Polyphonic VST3 & Standalone Synthesizer
; Developed by LU5 (https://lu5.art.br)
; ==============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "1.1.1"
#endif
#ifndef BuildType
  #define BuildType "Release"
#endif

#define MyAppName "Md-101"
#define MyAppPublisher "LU5"
#define MyAppURL "https://lu5.art.br"
#define MyAppExeName "Md-101.exe"

[Setup]
AppId={{8D101A5E-2580-4101-9B18-101258AE1010}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppName}
LicenseFile=..\..\LICENSE
OutputDir=..\..\dist
OutputBaseFilename=Md-101-v{#MyAppVersion}-Windows-x64-Setup
SetupIconFile=md-101.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Languages]
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (VST3 Plugin + Standalone + Manuals)"
Name: "vst3only"; Description: "VST3 Plugin only (for DAWs)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin (64-bit) — C:\Program Files\Common Files\VST3\Md-101.vst3"; Types: full vst3only custom
Name: "standalone"; Description: "Standalone Application (Md-101.exe)"; Types: full custom
Name: "docs"; Description: "User Manuals (HTML & Markdown — PT/EN)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: standalone

[Files]
; VST3 Plugin Bundle
Source: "..\..\build\Md101_artefacts\{#BuildType}\VST3\Md-101.vst3\*"; DestDir: "{autocs64}\VST3\Md-101.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone Application
Source: "..\..\build\Md101_artefacts\{#BuildType}\Standalone\Md-101.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "md-101.ico"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
; Documentation & License
Source: "..\..\README.md"; DestDir: "{app}"; Components: docs; Flags: ignoreversion
Source: "..\..\README-en.md"; DestDir: "{app}"; Components: docs; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; Components: docs; Flags: ignoreversion
Source: "..\..\docs\*"; DestDir: "{app}\docs"; Components: docs; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\md-101.ico"; Components: standalone
Name: "{group}\{#MyAppName} — Manual (PT)"; Filename: "{app}\docs\manual.html"; Components: docs
Name: "{group}\{#MyAppName} — Manual (EN)"; Filename: "{app}\docs\manual-en.html"; Components: docs
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\md-101.ico"; Tasks: desktopicon; Components: standalone

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent; Components: standalone
