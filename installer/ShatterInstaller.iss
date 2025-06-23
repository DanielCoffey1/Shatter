[Setup]
AppName=Shatter
AppVersion=1.0.0
DefaultDirName={autopf}\Shatter
DefaultGroupName=Shatter
OutputDir=C:\Users\Daniel\Desktop\ShatterInstaller
OutputBaseFilename=ShatterInstaller
Compression=lzma
SolidCompression=yes
UninstallDisplayIcon={app}\Shatter.exe
SetupIconFile=res\shatter.ico
AppPublisher=Daniel Coffey
AppPublisherURL=https://github.com/danielcoffey1/Shatter
LicenseFile=license.txt



[Files]
Source: "Shatter.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Shatter.ini"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Shatter"; Filename: "{app}\Shatter.exe"

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Shatter"; ValueData: """{app}\Shatter.exe"""; Flags: uninsdeletevalue

[Run]
Filename: "{app}\Shatter.exe"; Description: "Launch Shatter"; Flags: nowait postinstall skipifsilent
