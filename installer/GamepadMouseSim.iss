[Setup]
AppId={{B1E2A3C4-D5E6-7890-ABCD-EF1234567890}
AppName=Gamepad Mouse Simulator
AppVersion=1.0.0
AppPublisher=MiMo
DefaultDirName={autopf}\GamepadMouseSim
DefaultGroupName=Gamepad Mouse Simulator
OutputDir=installer
OutputBaseFilename=GamepadMouseSim-setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\build\GamepadMouseSim.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion
Source: "..\config\default_config.json"; DestDir: "{app}\config"; Flags: ignoreversion

[Icons]
Name: "{group}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"
Name: "{group}\{cm:UninstallProgram,Gamepad Mouse Simulator}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\GamepadMouseSim.exe"; Description: "{cm:LaunchProgram,Gamepad Mouse Simulator}"; Flags: nowait postinstall
