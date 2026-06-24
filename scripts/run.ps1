# Run Agriculture Sensor Dashboard
# Usage: .\scripts\run.ps1 [-Config Release|Debug] [-Deploy] [-NoBuild]

param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$Deploy,
    [switch]$NoBuild
)

. "$PSScriptRoot\common.ps1"

$ProjectRoot = Get-ProjectRoot
$BuildDir = Get-BuildDir -ProjectRoot $ProjectRoot
$QtDir = Find-QtInstall -ProjectRoot $ProjectRoot

Write-Host "=== Agriculture Sensor Dashboard - Run ===" -ForegroundColor Green

if (-not $NoBuild) {
    $exeCheck = Get-ExecutablePath -BuildDir $BuildDir -Config $Config
    if (-not $exeCheck) {
        Write-Host "Executable not found. Building first..." -ForegroundColor Yellow
        & "$PSScriptRoot\build.ps1" -Config $Config
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

$exe = Get-ExecutablePath -BuildDir $BuildDir -Config $Config
if (-not $exe) {
    Write-Host "Executable not found. Run .\scripts\build.ps1 first." -ForegroundColor Red
    exit 1
}

if ($Deploy -or -not $env:AGRI_DASH_SKIP_DEPLOY) {
    if ($QtDir) {
        Write-Host "Deploying Qt runtime libraries..." -ForegroundColor Cyan
        Invoke-QtDeploy -QtDir $QtDir -ExePath $exe
    } elseif (-not $env:QT_DIR) {
        Write-Warning "Qt path unknown; if the app fails to start, set QT_DIR or create .qt-path"
    }
}

Write-Host "Starting: $exe" -ForegroundColor Cyan
Write-Host ""

$exeDir = Split-Path $exe -Parent
Push-Location $exeDir
try {
    & $exe
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
