# Shared helpers for Agriculture Sensor Dashboard build scripts.

$ErrorActionPreference = "Stop"

function Get-ProjectRoot {
    if ($PSScriptRoot) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }
    return (Get-Location).Path
}

function Find-QtInstall {
    param(
        [string]$ProjectRoot
    )

    $candidates = @()

    if ($env:QT_DIR) {
        $candidates += $env:QT_DIR
    }

    $localConfig = Join-Path $ProjectRoot ".qt-path"
    if (Test-Path $localConfig) {
        $line = (Get-Content $localConfig -ErrorAction SilentlyContinue | Where-Object { $_ -and -not $_.StartsWith("#") } | Select-Object -First 1)
        if ($line) { $candidates += $line.Trim() }
    }

    $searchRoots = @(
        "C:\Qt",
        "$env:USERPROFILE\Qt",
        "$env:LOCALAPPDATA\Qt"
    )

    foreach ($root in $searchRoots) {
        if (-not (Test-Path $root)) { continue }
        $configs = Get-ChildItem -Path $root -Recurse -Filter "Qt6Config.cmake" -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "lib[\\/]cmake[\\/]Qt6[\\/]Qt6Config\.cmake$" }
        foreach ($cfg in $configs) {
            $kitRoot = $cfg.Directory.Parent.Parent.Parent.FullName
            $candidates += $kitRoot
        }
    }

  $preferred = @("msvc2019_64", "msvc2022_64", "mingw_64")
    $unique = $candidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique

    foreach ($name in $preferred) {
        $match = $unique | Where-Object { $_ -match [regex]::Escape($name) } | Select-Object -First 1
        if ($match) { return $match }
    }

    return ($unique | Select-Object -First 1)
}

function Setup-QtToolchain {
    param([string]$QtDir)

    $qtBin = Join-Path $QtDir "bin"
    if (Test-Path $qtBin) {
        $env:PATH = "$qtBin;$env:PATH"
    }

    if ($QtDir -match "mingw") {
        $mingwRoots = @(
            "C:\Qt\Tools\mingw1310_64",
            "C:\Qt\Tools\mingw1120_64",
            "C:\Qt\Tools\mingw_64"
        )
        foreach ($root in $mingwRoots) {
            $gcc = Join-Path $root "bin\g++.exe"
            if (Test-Path $gcc) {
                $env:PATH = "$(Join-Path $root 'bin');$env:PATH"
                return @{
                    Generator = "Ninja"
                    ExtraArgs = @(
                        "-DCMAKE_C_COMPILER=$(Join-Path $root 'bin\gcc.exe')",
                        "-DCMAKE_CXX_COMPILER=$gcc"
                    )
                }
            }
        }
    }

    return @{ Generator = $null; ExtraArgs = @() }
}

function Get-CmakeConfigureArgs {
    param(
        [string]$ProjectRoot,
        [string]$QtDir,
        [string]$Config
    )

    $toolchain = Setup-QtToolchain -QtDir $QtDir
    $args = @(
        $ProjectRoot,
        "-DCMAKE_PREFIX_PATH=$QtDir",
        "-DCMAKE_BUILD_TYPE=$Config"
    )

    if ($toolchain.Generator) {
        $ninja = "C:\Qt\Tools\Ninja\ninja.exe"
        if (Test-Path $ninja) {
            $env:PATH = "$(Split-Path $ninja -Parent);$env:PATH"
        }
        $args = @("-G", $toolchain.Generator) + $args + $toolchain.ExtraArgs
    }

    return $args
}

function Stop-RunningApp {
    param([string]$BuildDir)

    $exe = Get-ExecutablePath -BuildDir $BuildDir
    if (-not $exe) { return }

    $procName = [System.IO.Path]::GetFileNameWithoutExtension($exe)
    $running = Get-Process -Name $procName -ErrorAction SilentlyContinue
    if ($running) {
        Write-Host "Closing running $procName (required to replace .exe)..." -ForegroundColor Yellow
        $running | Stop-Process -Force
        Start-Sleep -Milliseconds 500
    }
}

function Get-BuildDir {
    param([string]$ProjectRoot)
    return (Join-Path $ProjectRoot "build")
}

function Get-ExecutablePath {
    param(
        [string]$BuildDir,
        [string]$Config = "Release"
    )

    $paths = @(
        (Join-Path $BuildDir "$Config\AgricultureSensorDashboard.exe"),
        (Join-Path $BuildDir "AgricultureSensorDashboard.exe")
    )

    foreach ($p in $paths) {
        if (Test-Path $p) { return (Resolve-Path $p).Path }
    }

    return $null
}

function Invoke-QtDeploy {
    param(
        [string]$QtDir,
        [string]$ExePath
    )

    $windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
    if (-not (Test-Path $windeployqt)) {
        Write-Warning "windeployqt not found. Adding Qt bin to PATH for this session."
        $env:PATH = "$(Join-Path $QtDir 'bin');$env:PATH"
        return
    }

    & $windeployqt --no-translations --no-compiler-runtime $ExePath
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed with exit code $LASTEXITCODE"
    }
}

function Write-BuildHelp {
    Write-Host ""
    Write-Host "Qt 6 was not found automatically." -ForegroundColor Yellow
    Write-Host "Set one of the following, then run build again:" -ForegroundColor Yellow
    Write-Host "  1. Environment variable:  `$env:QT_DIR = 'C:\Qt\6.8.0\msvc2019_64'" -ForegroundColor Cyan
    Write-Host "  2. Local file:            create .qt-path with your Qt kit path" -ForegroundColor Cyan
    Write-Host ""
}
