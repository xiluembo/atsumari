[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$AppExe,

    [Parameter(Mandatory)]
    [string]$QtBin,

    [Parameter(Mandatory)]
    [string]$Iscc,

    [Parameter(Mandatory)]
    [ValidatePattern('^[0-9][0-9A-Za-z.+-]*$')]
    [string]$Version,

    [Parameter(Mandatory)]
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9_-]*$')]
    [string]$KitTag,

    [Parameter(Mandatory)]
    [string]$OutputDir,

    [string]$QmlDir,

    [string]$CompilerBin
)

$ErrorActionPreference = 'Stop'

$appPath = (Resolve-Path -LiteralPath $AppExe).ProviderPath
$qtBinPath = (Resolve-Path -LiteralPath $QtBin).ProviderPath
$qtRoot = Split-Path -Parent $qtBinPath
$qtPluginPath = Join-Path $qtRoot 'plugins'
$platformPlugin = Join-Path $qtPluginPath 'platforms\qwindows.dll'

if (-not (Test-Path -LiteralPath $platformPlugin -PathType Leaf)) {
    throw "Plugin de plataforma do kit ausente: $platformPlugin"
}

$deployPath = @($qtBinPath)
if ($CompilerBin) {
    $deployPath += (Resolve-Path -LiteralPath $CompilerBin).ProviderPath
}

$winDeployQt = (Resolve-Path -LiteralPath (
    Join-Path $qtBinPath 'windeployqt.exe'
)).ProviderPath
$isccPath = (Resolve-Path -LiteralPath $Iscc).ProviderPath
$issPath = Join-Path $PSScriptRoot 'Atsumari.iss'

$appName = Split-Path -Leaf $appPath
$outputPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath(
    $OutputDir
)
$installerName = "Atsumari-$Version-$KitTag-Setup"

$deployArgs = @('--verbose', '2')
if ($QmlDir) {
    $qmlPath = (Resolve-Path -LiteralPath $QmlDir).ProviderPath
    $deployArgs += @('--qmldir', $qmlPath)
}

$stagePath = Join-Path ([System.IO.Path]::GetTempPath()) (
    'Atsumari-' + [guid]::NewGuid().ToString('N')
)
$originalPath = $env:Path

$originalPluginPath = $env:QT_PLUGIN_PATH

try {
    New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
    New-Item -ItemType Directory -Path $stagePath | Out-Null

    $stagedExe = Join-Path $stagePath $appName
    Copy-Item -LiteralPath $appPath -Destination $stagedExe
    $deployArgs += $stagedExe

    $env:Path = ($deployPath + $originalPath) -join ';'
    $env:QT_PLUGIN_PATH = $qtPluginPath
    & $winDeployQt @deployArgs
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt falhou com código $LASTEXITCODE."
    }

    $innoArgs = @(
        "/DStageDir=$stagePath"
        "/DAppExe=$appName"
        "/DAppVersion=$Version"
        "/DKitTag=$KitTag"
        "/O$outputPath"
        "/F$installerName"
        $issPath
    )
    & $isccPath @innoArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Inno Setup falhou com código $LASTEXITCODE."
    }

    Write-Host "Instalador gerado em $(Join-Path $outputPath "$installerName.exe")"
}
finally {
    $env:Path = $originalPath
	$env:QT_PLUGIN_PATH = $originalPluginPath
    if (Test-Path -LiteralPath $stagePath) {
        Remove-Item -LiteralPath $stagePath -Recurse -Force
    }
}