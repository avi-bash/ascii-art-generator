#define AppName "ASCII Art Generator"
#define AppVersion "1.0.0"
#define AppPublisher "ASCII Art Generator"
#define AppExeName "ascii-translation.exe"

[Setup]
AppId={{B89A3D63-1F0F-4F26-9BB4-ASCIIARTGEN01}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\ASCII Art Generator
DefaultGroupName={#AppName}
OutputDir={#OutputDir}
OutputBaseFilename=ascii-translation-setup
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppExeName}

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\ascii-translation.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\ASCII Art Generator"; Filename: "{app}\ascii-translation.exe"; WorkingDir: "{app}"

[UninstallDelete]
Type: filesandordirs; Name: "{app}"