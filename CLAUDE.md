# CLAUDE.md — contexte pour Claude Code

Lu automatiquement par Claude Code. Résume ce qu'il faut savoir avant de toucher au dépôt. Le propriétaire (Guyno) n'est pas codeur : on lui parle en français sans jargon, on lui demande des choix et des tests en casque, jamais de code.

## Le projet en trois lignes

RetroXR = module OpenXR ajouté à **libultraship** (bibliothèque commune des ports PC Harbour Masters) pour jouer aux jeux N64 décompilés en VR sur Meta Quest 3. Premier jeu : **Mario 64 via Ghostship**. Plan B : Zelda OoT via Ship of Harkinian. On ne réécrit pas les jeux ; on patche la bibliothèque et on fournit un module `xr/`.

## Décisions figées (ne pas rouvrir sans l'accord de Guyno)

- Base technique : **Kenix3/libultraship @ 93a1a41** (celle épinglée par Ghostship), PAS HarbourMasters/libultraship main (SDL3, transform CPU : incompatible). Ghostship @ 40eef13 (branche `develop`, 11 sept 2026).
- V1 = OpenGL + `XR_KHR_opengl_enable` (contexte SDL2 déjà là). DX11 en V2, Vulkan hors scope.
- V1 ne touche pas au code du jeu : tout passe par `Fast3dWindow::DrawAndRunGraphicsCommands` (double rendu) et `Interpreter::GfxSpMatrix` (substitution projection + tête).
- Jamais de code copié de RaYRoD/MVRH (GPLv2) : on s'inspire de l'architecture de `vr.h`, on réécrit.
- Zéro ROM/asset/clé dans le dépôt. Licences : MIT (code), CC-BY-4.0 (docs, profils).

## Où sont les choses (chemins dans les projets d'origine)

Étude complète : `docs/ETUDE-TECHNIQUE.md`. Points d'ancrage :
- Boucle de frame : `libultraship/src/fast/Fast3dWindow.cpp` `DrawAndRunGraphicsCommands` (~l.232).
- Matrices (point unique) : `libultraship/src/fast/interpreter.cpp` `GfxSpMatrix` (~l.2255-2310 dans le commit Kenix3). Perspective si `P[2][3] != 0`, ortho si `P[3][3]==1 && P[2][3]==0`.
- Render-to-texture : `Interpreter::StartFrame`, `mGameFb`, `mRendersToFb` (à forcer à true en XR), `CreateFramebuffer` / `CopyFramebuffer` / `GetFramebufferTextureId` dans `include/fast/backends/gfx_rendering_api.h`.
- Fenêtre/GL : `src/fast/backends/gfx_sdl2.cpp` (`SwapBuffersBegin` → court-circuiter `SyncFramerateWithTime` en XR).
- Entrées : `ControlDeck::WriteToOSContPad`, `Controller::ReadToOSContPad`, factories de mapping (`ButtonMappingFactory.cpp`), `PhysicalDeviceType.h` (ajouter `OpenXR`).
- Caméra Mario : `Ghostship/src/game/camera.c`, `rendering_graph_node.c` (`geo_process_camera` l.343, `geo_process_perspective` l.256). V2 seulement.
- Ghostship : `src/port/Engine.cpp` (boucle, `InterpolationFPS` l.1541), `CMakeLists.txt` (`vcpkg_install_packages` l.84 → ajouter `openxr-loader`), options `BUILD_SM64/MK64/SF64/FZERO`.

## Organisation de ce dépôt

- `xr/` : notre module (C++17, API C publique `xr/xr.h` sans fuite de types OpenXR/GL). Copié dans `libultraship/src/xr/` par `scripts/bootstrap.ps1`.
- `patches/libultraship/*.patch`, `patches/ghostship/*.patch` : diffs `git format-patch` sur les commits épinglés ci-dessus. Petits, lisibles, numérotés.
- `scripts/bootstrap.ps1` : clone Ghostship au commit épinglé (+ submodules), applique les patches, copie `xr/`. `scripts/build.ps1` : CMake + build Release. CI Windows = ces deux scripts.
- `profiles/<jeu>/profile.json` : confort (unités par mètre, rotation, HUD, mapping).
- `docs/ROADMAP.md` : cases à cocher ; c'est ce que Guyno lit.

## Conventions

- Préfixe `xr_` pour l'API C, namespace `RetroXR` en C++. Define `ENABLE_XR` ; sans lui, tout est no-op inline.
- Logs : `SPDLOG_INFO("[RetroXR] ...")`. Les logs sont la seule source de vérité : Guyno ne peut que coller `ghostship.log` et décrire ce qu'il voit dans le casque.
- Chaque patch doit s'appliquer proprement (`git apply --check`) sur le commit épinglé. La CI le vérifie.
- Commits sobres, en français, sans ligne d'attribution.
- Ne jamais demander à Guyno d'ouvrir un IDE ou de lire du code : lui donner un `.exe` (artefact CI) et une procédure en 5 lignes.

## Quand ça plante en casque

1. Demander `ghostship.log` (dossier du jeu) et une description : écran noir ? une image double ? fenêtre PC ok mais rien dans le casque ?
2. Vérifier d'abord que le runtime OpenXR actif est Meta Quest Link (`HKLM\SOFTWARE\Khronos\OpenXR\1\ActiveRuntime`).
3. Réduire : XR désactivé (cvar `gXR.Enabled=0`) → jeu normal ok ? puis XR sans double rendu → puis complet.
