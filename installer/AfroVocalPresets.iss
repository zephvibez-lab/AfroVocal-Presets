; AfroVocal Presets — Windows x64 VST3 installer
; Built by Inno Setup on the native Windows GitHub Actions runner.

#define MyAppName "AfroVocal Presets"
#define MyAppVersion "0.6.0"
#define MyAppPublisher "Personal Audio Tools"
#define MyAppURL "https://github.com/zephvibez-lab/AfroVocal-Presets"
#define MyAppExeName "AfroVocal Presets.vst3"

[Setup]
AppId={{D2E3B917-CE6C-4C70-8B4A-0A7F7B40E5A1}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commoncf64}\VST3\AfroVocal Presets.vst3
DisableProgramGroupPage=yes
DisableDirPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=release
OutputBaseFilename=AfroVocal-Presets-Setup-x64
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
Uninstallable=yes
UninstallDisplayName={#MyAppName} VST3
UninstallDisplayIcon={app}\Contents\x86_64-win\AfroVocal Presets.vst3
CloseApplications=prompt
RestartApplications=no
VersionInfoVersion={#MyAppVersion}.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Windows VST3 plug-in
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "release\AfroVocal Presets.vst3\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := IsWin64;
  if not Result then
    MsgBox('AfroVocal Presets requires 64-bit Windows.', mbError, MB_OK);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  NeedsRestart := False;
end;
