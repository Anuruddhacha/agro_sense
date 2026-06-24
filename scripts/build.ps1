# Build Agriculture Sensor Dashboard (Qt 6 + CMake)
# Usage: .\scripts\build.ps1 [-Config Release|Debug] [-Rebuild]

param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$Rebuild
)

. "$PSScriptRoot\common.ps1"

$ProjectRoot = Get-ProjectRoot
$BuildDir = Get-BuildDir -ProjectRoot $ProjectRoot
$QtDir = Find-QtInstall -ProjectRoot $ProjectRoot

Write-Host "=== Agriculture Sensor Dashboard - Build ===" -ForegroundColor Green
Write-Host "Project: $ProjectRoot"
Write-Host "Config:  $Config"

if (-not $QtDir) {
    Write-BuildHelp
    exit 1
}

Write-Host "Qt:      $QtDir"
Write-Host "Build:   $BuildDir"
Write-Host ""

if ($Rebuild -and (Test-Path $BuildDir)) {
    Write-Host "Removing existing build directory..." -ForegroundColor Yellow
    Remove-Item $BuildDir -Recurse -Force
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Push-Location $BuildDir
try {
    if (-not (Test-Path "CMakeCache.txt")) {
        Write-Host "Configuring CMake..." -ForegroundColor Cyan
        $cmakeArgs = Get-CmakeConfigureArgs -ProjectRoot $ProjectRoot -QtDir $QtDir -Config $Config
        & cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    } else {
        Write-Host "Using existing CMake cache (use -Rebuild for a clean build)" -ForegroundColor DarkGray
    }

    Write-Host "Building..." -ForegroundColor Cyan
    Setup-QtToolchain -QtDir $QtDir | Out-Null
    cmake --build . --config $Config --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    $exe = Get-ExecutablePath -BuildDir $BuildDir -Config $Config
    if (-not $exe) {
        throw "Build finished but executable was not found."
    }

    Write-Host ""
    Write-Host "Build succeeded." -ForegroundColor Green
    Write-Host "Executable: $exe"
    Write-Host ""
    Write-Host "Run with:  .\scripts\run.ps1" -ForegroundColor Cyan
    Write-Host "       or: .\run.bat" -ForegroundColor Cyan
}
finally {
    Pop-Location
}
