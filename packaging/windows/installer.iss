; Inno Setup script for lusakey (Windows installer).
;
; Before compiling this script:
;   1. Build lusakey in Release mode (cmake --build --preset windows-x64 --config Release).
;   2. Run windeployqt on the built lusakey.exe so all required Qt6 DLLs,
;      platform plugins, and QML modules are staged alongside it — this
;      script only packages what's already in SourceDir below, it doesn't
;      run windeployqt itself.
;   3. Compile with: iscc installer.iss   (or open in the Inno Setup IDE)
;
; No code-signing directive is included here — sign the built lusakey.exe
; AND the installer .exe this script produces with signtool separately,
; using a purchased Authenticode/EV certificate (see packaging/README.md).
; Unsigned builds will trigger a Windows SmartScreen warning on first run.

#define AppName "lusakey"
#define AppVersion "0.1.0"
#define AppPublisher "lusakey"
#define AppExeName "lusakey.exe"
#define SourceDir "..\..\build\windows-x64\app"

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
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
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
