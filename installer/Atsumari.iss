[Setup]
AppId=Atsumari-{#KitTag}
AppName=Atsumari ({#KitTag})
AppVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\Atsumari\{#KitTag}
PrivilegesRequired=lowest
WizardStyle=modern
Compression=lzma2
SolidCompression=yes

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Atsumari ({#KitTag})"; Filename: "{app}\{#AppExe}"; WorkingDir: "{app}"
