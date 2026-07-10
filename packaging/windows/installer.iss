; Inno Setup script for lusakey (Windows installer).
;
; Not meant to be run by hand — scripts\windows\build-release.ps1 builds the
; Release config, stages it with windeployqt into SourceDir below, and then
; invokes `iscc installer.iss` as its last step. Run that script instead of
; this file directly unless you've already done the equivalent staging.
;
; No code-signing directive is included here — sign the built lusakey.exe
; AND the installer .exe this script produces with signtool separately,
; using a purchased Authenticode/EV certificate (see packaging/README.md).
; Unsigned builds will trigger a Windows SmartScreen warning on first run.

#define AppName "lusakey"
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#define AppPublisher "lusakey"
#define AppExeName "lusakey.exe"
#define SourceDir "..\..\build\windows-x64-release\dist\lusakey"

[Setup]
AppId={{6C3F3E2A-6B7B-4E90-9C8E-8E9C7A9B9C10}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
OutputDir=..\..\build\installers
OutputBaseFilename=lusakey-{#AppVersion}-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent
