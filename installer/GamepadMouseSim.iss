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
Source: "..\dist\GamepadMouseSim.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion
Source: "..\dist\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion
Source: "..\dist\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion
Source: "..\dist\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion
Source: "..\dist\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion
Source: "..\dist\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion
Source: "..\dist\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion
Source: "..\dist\config\default_config.json"; DestDir: "{app}\config"; Flags: ignoreversion

[Icons]
Name: "{group}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"
Name: "{group}\{cm:UninstallProgram,Gamepad Mouse Simulator}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Gamepad Mouse Simulator"; Filename: "{app}\GamepadMouseSim.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\GamepadMouseSim.exe"; Description: "{cm:LaunchProgram,Gamepad Mouse Simulator}"; Flags: nowait postinstall
