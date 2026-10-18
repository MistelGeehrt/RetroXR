# RetroXR — compile Ghostship + module xr/ (Windows, Visual Studio 2022+, CMake, Python).
# Usage : .\scripts\build.ps1 [-Config Release|Debug] [-NoXR]
# Résultat : work\Ghostship\build\x64\<Config>\Ghostship.exe

[CmdletBinding()]
param(
    [string]$Config = "Release",
    [switch]$NoXR,
    [string]$WorkDir = (Join-Path $PSScriptRoot "..\work")
)
$ErrorActionPreference = "Stop"
$Game = Join-Path $WorkDir "Ghostship"
if (-not (Test-Path $Game)) { throw "[RetroXR] lance d'abord .\scripts\bootstrap.ps1" }

$xr = if ($NoXR) { "OFF" } else { "ON" }
$build = Join-Path $Game "build\x64"

Write-Host "[RetroXR] configure (LUS_ENABLE_XR=$xr)"
cmake -S $Game -B $build -G "Visual Studio 17 2022" -T v143 -A x64 -DLUS_ENABLE_XR=$xr -DUSE_SATELLA=OFF -DUSE_NETWORKING=OFF
if ($LASTEXITCODE -ne 0) { throw "configure a échoué" }

Write-Host "[RetroXR] build $Config"
cmake --build $build --config $Config --parallel
if ($LASTEXITCODE -ne 0) { throw "build a échoué" }

$exe = Get-ChildItem (Join-Path $build $Config) -Filter *.exe -Recurse | Select-Object -First 1
Write-Host "[RetroXR] ok : $($exe.FullName)"
