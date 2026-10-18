# RetroXR — récupère Ghostship (Mario 64) au commit épinglé, applique nos patches, copie le module xr/.
# Usage (PowerShell, à la racine du dépôt RetroXR) :
#   .\scripts\bootstrap.ps1            # crée .\work\Ghostship
#   .\scripts\bootstrap.ps1 -Force     # repart de zéro
# Prérequis : git. Aucune ROM n'est nécessaire à cette étape.

[CmdletBinding()]
param(
    [switch]$Force,
    [string]$WorkDir = (Join-Path $PSScriptRoot "..\work")
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Pins = Get-Content (Join-Path $Root "scripts\pins.json") | ConvertFrom-Json
$Game = Join-Path $WorkDir "Ghostship"

if ($Force -and (Test-Path $Game)) { Remove-Item -Recurse -Force $Game }
New-Item -ItemType Directory -Force $WorkDir | Out-Null

if (-not (Test-Path (Join-Path $Game ".git"))) {
    Write-Host "[RetroXR] clone Ghostship @ $($Pins.ghostship.commit)"
    git clone --no-checkout $Pins.ghostship.url $Game
    git -C $Game checkout --quiet $Pins.ghostship.commit
    git -C $Game submodule update --init --recursive --depth 1 2>$null
    if ($LASTEXITCODE -ne 0) { git -C $Game submodule update --init --recursive }
}

# Vérifie que libultraship est bien au commit attendu (Kenix3 fork)
$lus = Join-Path $Game "libultraship"
$lusHead = git -C $lus rev-parse HEAD
if ($lusHead -ne $Pins.libultraship.commit) {
    throw "[RetroXR] libultraship est à $lusHead, attendu $($Pins.libultraship.commit). Ghostship a changé d'épingle : mettre à jour scripts/pins.json et les patches."
}

# Remet les deux arbres à l'état d'origine avant d'appliquer (idempotent)
git -C $Game checkout -- . ; git -C $Game clean -fdq -e work
git -C $lus  checkout -- . ; git -C $lus  clean -fdq

function Apply-Patches($repo, $dir) {
    $patches = Get-ChildItem (Join-Path $Root $dir) -Filter *.patch -ErrorAction SilentlyContinue | Sort-Object Name
    foreach ($p in $patches) {
        Write-Host "[RetroXR] patch $($p.Name) -> $dir"
        git -C $repo apply --check $p.FullName
        git -C $repo apply --whitespace=nowarn $p.FullName
    }
}
Apply-Patches $lus  "patches\libultraship"
Apply-Patches $Game "patches\ghostship"

# Copie du module xr/ dans libultraship/src/xr
$dst = Join-Path $lus "src\xr"
New-Item -ItemType Directory -Force $dst | Out-Null
Copy-Item (Join-Path $Root "xr\*") $dst -Recurse -Force

Write-Host "[RetroXR] prêt : $Game"
Write-Host "[RetroXR] étape suivante : .\scripts\build.ps1"
