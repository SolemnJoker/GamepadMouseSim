[Setup]
AppId={{B1E2A3C4-D5E6-7890-ABCD-EF1234567890}
AppName=Gamepad Mouse Simulator
AppVersion=1.0.0
AppPublisher=SolemnJoker
DefaultDirName={autopf}\GamepadMouseSim
DefaultGroupName=Gamepad Mouse Simulator
OutputDir=.
OutputBaseFilename=GamepadMouseSim-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "autostart"; Description: "开机自动启动 Gamepad Mouse Simulator"; GroupDescription: "其他选项:"; Flags: unchecked

; The build tree contains whatever runtime DLLs the building toolchain
; produced (MSVC: vcruntime/msvcp set, MinGW: libgcc/libstdc++/libwinpthread).
; Wildcarding keeps the installer independent of which machine built it.
[Files]
Source: "..\dist\GamepadMouseSim.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion
Source: "..\dist\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion
Source: "..\dist\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion
Source: "..\dist\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion
Source: "..\dist\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion
Source: "..\dist\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion
Source: "..\dist\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion

[Icons]
Name: "{group}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"
Name: "{group}\{cm:UninstallProgram,Gamepad Mouse Simulator}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"; Tasks: desktopicon

; Optional auto-start (unchecked by default). Writes HKCU\...\Run pointing at
; the installed exe; on first launch the app adopts it into config.json
; (autostart: true) so GUI and registry stay consistent. uninsdeletevalue
; removes the entry when the app is uninstalled.
[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "GamepadMouseSim"; ValueData: """{app}\GamepadMouseSim.exe"""; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\GamepadMouseSim.exe"; Description: "{cm:LaunchProgram,Gamepad Mouse Simulator}"; Flags: nowait postinstall
