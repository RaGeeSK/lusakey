[Setup]
AppId={{B5F8CBE2-0D5C-4C2B-9E0C-1D6A0B3B4C2E}}
AppName=LusaKey
AppVersion=1.0.0
DefaultDirName={pf}\LusaKey
DefaultGroupName=LusaKey
OutputDir=.
OutputBaseFilename=LusaKeySetup
Compression=lzma
SolidCompression=yes
DisableProgramGroupPage=yes

[Files]
Source: "..\build\lusakey.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\LusaKey"; Filename: "{app}\lusakey.exe"
Name: "{group}\Uninstall LusaKey"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\lusakey.exe"; Description: "Launch LusaKey"; Flags: nowait postinstall skipifsilent
