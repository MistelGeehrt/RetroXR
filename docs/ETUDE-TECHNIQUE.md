<!-- Rapport d'étude du 2026-10-18, base de la conception RetroXR. Lecture réservée aux contributeurs techniques. -->

# Rapport — couche OpenXR pour libultraship / Ghostship (Quest 3 via Link)

Dépôts clonés dans `/tmp/claude-0/-home-claude/79546b64-c95a-5b11-8a81-b0cc4a492085/scratchpad/study/` :
- `libultraship/` = HarbourMasters/libultraship `main` (759b134, 2 sept 2026).
- `lus-ghostship/` = worktree sur le commit **réellement utilisé par Ghostship** (93a1a41, 8 sept 2026, fork Kenix3/libultraship, cf. `.gitmodules`). C'est ce code-là qui compte.
- `Ghostship/` = branche par défaut `develop` (40eef13, 11 sept 2026).
- `MVRH/` (main) et `MVRH-gpl/` (branche gpl-source).
- **MarioKart64-VR : non public** (404/403 sur clone, API et add_repo). Rien à lire.

## Point crucial découvert : Ghostship ne suit pas libultraship main

`git diff --stat HEAD FETCH_HEAD` : 252 fichiers, +14 131/-9 177 lignes entre main et le commit épinglé. Différences qui changent la conception VR :
- main utilise **SDL3** (`src/fast/backends/gfx_sdl.cpp`) ; le fork Kenix3 utilise **SDL2** (`gfx_sdl2.cpp`) et ajoute un **backend Vulkan** (`gfx_vulkan.cpp`, 2 418 lignes).
- Dans main, la transformation des sommets se fait **sur CPU** : `Interpreter::GfxSpVertex` (`src/fast/interpreter.cpp:1612-1619`) multiplie chaque sommet par `MP_matrix` et le shader fait `gl_Position = aVtxPos;` (`shaders/opengl/default.shader.glsl:76`).
- Dans le fork Kenix3, la transformation est **sur GPU** avec une palette de matrices : `GfxSpVertex` (`lus-ghostship/src/fast/interpreter.cpp:2354-2378`) stocke la position objet + `mtx_slot`, `AppendMtxHistory` (l.160) remplit `mMtxHistory`, et le vertex shader (`shaders/opengl/include/fast3d_vs.glsli:67,167`) fait `clipPos = aVtxPos * uMtxPalette[mtxBase..+3]`. Il y a aussi un étage de post-process (`RunPostPasses()`, l.6747).

Conclusion : forker le commit 93a1a41 (Kenix3), pas HM main. Les numéros de lignes ci-dessous renvoient à main sauf mention `lus-ghostship`.

## 1. Architecture de rendu (Fast3D)

**Boucle de frame.** `Fast3dWindow::DrawAndRunGraphicsCommands` (`src/fast/Fast3dWindow.cpp:232-253`) : `gui->StartDraw()` → `mInterpreter->StartFrame()` → `mInterpreter->Run(commands, mtxReplacements)` → `gui->EndDraw()` → `mInterpreter->EndFrame()`. `EndFrame` (`interpreter.cpp:5241`) = `mRapi->EndFrame(); mWapi->SwapBuffersBegin(); mRapi->FinishRender(); mWapi->SwapBuffersEnd();`. Côté SDL, `SwapBuffersBegin` (`gfx_sdl.cpp:757-766`) fait `SyncFramerateWithTime()` puis `SDL_GL_SwapWindow`. Côté DXGI, `swap_chain->Present` (`gfx_dxgi.cpp:946`) avec pacing par timer waitable.

**Backends** (`Fast3dWindow::InitWindowManager`, l.162-198) : `GfxWindowBackendDXGI + GfxRenderingAPIDX11` (Windows, `ENABLE_DX11` forcé dans `src/CMakeLists.txt:199-206`), `GfxWindowBackendSDL + GfxRenderingAPIOGL` (`ENABLE_OPENGL`), Metal (Apple). Fork Kenix3 : + `GfxRenderingAPIVK`. Interface commune : `include/fast/backends/gfx_rendering_api.h` (`CreateFramebuffer`, `UpdateFramebufferParameters`, `StartDrawToFramebuffer`, `CopyFramebuffer`, `ResolveMSAAColorBuffer`, `GetFramebufferTextureId`). Le choix est persisté dans `Window.Backend.Id` (`src/ship/window/Window.cpp:143`).

**Matrices.** Point unique : `Interpreter::GfxSpMatrix` (`interpreter.cpp:1507-1562` ; `lus-ghostship` l.2255-2310). Convention N64 (vecteurs lignes) : `G_MTX_PROJECTION|G_MTX_LOAD` → `memcpy(mRsp->P_matrix, matrix)` (l.1541), `G_MTX_MUL` → `MatrixMul(P, matrix, P)` ; modelview sur une pile de 11 ; puis `MP_matrix = MV * P` (l.1561). Le jeu ne voit jamais de matrice GPU : SM64 envoie `guPerspective` (`Ghostship/src/game/rendering_graph_node.c:273-276`), un roll en `G_MTX_PROJECTION|G_MTX_MUL` (l.357) et la lookat caméra en modelview (l.359-363). Le remplacement `mtx_replacements` (l.1510, clé = pointeur `Mtx*`) sert à l'interpolation de frames de Ghostship et s'applique aussi aux matrices de projection.

**Render-to-texture.** `Interpreter::Init` crée `mGameFb` et `mGameFbMsaaResolved` (l.5005-5006). `StartFrame` (l.5083-5132) : si la résolution interne (`mCurDimensions`, pilotée par `CVAR_INTERNAL_RESOLUTION` et `Fast3dGui::ApplyResolutionChanges`, `Fast3dGui.cpp:458-513`) diffère du viewport ImGui ou si MSAA>1, `mRendersToFb = true` et le jeu est rendu dans `mGameFb`. `Run` (l.5176-5239) termine par `mGfxFrameBuffer = GetFramebufferTextureId(mGameFb | mGameFbMsaaResolved)` (GL : `GLuint clrbuf` en `GL_RGB8`, `gfx_opengl.cpp:815,917` ; DX11 : `ID3D11ShaderResourceView*`, `gfx_direct3d11.cpp:976`). `Fast3dGui::DrawGame` (`Fast3dGui.cpp:399-456`) affiche cette texture avec `ImGui::Image(fb, size)` dans la fenêtre "Main Game", avec letterbox/pillarbox calculé ici et `mGameWindowViewport` (l.363-366).

**Meilleurs points d'insertion.**
(a) Rendu double : au niveau de `Fast3dWindow::DrawAndRunGraphicsCommands` — remplacer l'appel unique `Run` par une boucle `for eye in {L,R}` : `mRapi->StartDrawToFramebuffer(eyeFb[eye])` puis `Run(commands, mtxReplacements)` avec un état `mXrEye` lu dans `GfxSpMatrix`. Ghostship fait déjà exactement cela pour l'interpolation : `GameEngine::RunCommands` (`Ghostship/src/port/Engine.cpp:1708-1720`) rejoue la même display list N fois par tick avec des `mtx` différents. Rendre deux fois coûte deux passes d'interprétation (display list SM64 : négligeable sur PC).
(b) Injection stéréo/tête : dans `GfxSpMatrix`, branche projection+load (l.1539-1544) : si la matrice est perspective (`P[2][3] != 0`), substituer `P_matrix = HeadRotInv_eye * EyeOffset * P_xr(fov asymétrique du runtime, near/far du jeu)`. Si ortho (`P[3][3]==1 && P[2][3]==0`, HUD via `create_dl_ortho_matrix`, `ingame_menu.c:239`, ou `geo_process_ortho_projection`), substituer `P = Ortho * Quad(d) * P_xr` pour poser le HUD sur un plan virtuel à ~2 m (sinon vision double). Un seul fichier, zéro modif du jeu pour la V1.
(c) Textures à soumettre : `GetFramebufferTextureId(eyeFb)` ou mieux `mRapi->CopyFramebuffer(xrSwapchainFb, eyeFb, …)` (GL : `glBlitFramebuffer` vers un FBO attaché à l'image de swapchain OpenXR). Les ids créés par `CreateFramebuffer` (l.5264) sont déjà redimensionnés par `StartFrame`.

## 2. Entrées

Chaîne : `LUS::ControlDeck::WriteToOSContPad` (`src/libultraship/controller/controldeck/ControlDeck.cpp:62-79`) → `Controller::ReadToOSContPad` (`src/libultraship/controller/controldevice/controller/Controller.cpp:28-66`) : pour chaque `ControllerButton` → `mapping->UpdatePad(padButtons)` ; sticks via `ControllerStick::Process` qui prend le max des `ControllerAxisDirectionMapping::GetNormalizedAxisDirectionValue()` (`ControllerStick.cpp:215-233`). Ghostship instancie `LUS::ControlDeck` dans `Engine.cpp:246`.

Deux façons de brancher des manettes OpenXR :
1. Propre : ajouter `PhysicalDeviceType::OpenXR` (`include/ship/controller/physicaldevice/PhysicalDeviceType.h:11-16`), des classes `XRButtonToButtonMapping` / `XRAxisDirectionToAxisDirectionMapping` (modèles : `mapping/sdl/SDLButtonToButtonMapping`, `SDLAxisDirectionToAxisDirectionMapping`), les enregistrer dans `ButtonMappingFactory::CreateButtonMappingFromConfig` (`factories/ButtonMappingFactory.cpp:14-80`) et `AxisDirectionMappingFactory`, ainsi que dans `Controller::AddDefaultMappings`. Les mappings lisent un état `xr::InputState` (boutons, sticks, poses) mis à jour par `xrSyncActions` une fois par frame. Les poses de manettes n'ont pas de place dans `OSContPad` (`include/libultraship/libultra/controller.h:107`, 0x24 octets) : exposer un accesseur C `xr_get_controller_pose()` pour le jeu.
2. Rapide : `ReadToOSContPad` fait `pad->button |=` et ne remplit les sticks que s'ils sont à 0 (l.48-62) ; un hook avant `WriteToPad` peut pré-remplir le pad. OK pour prototyper, pas pour livrer.

## 3. Caméra

Aucune caméra dans libultraship (grep "camera" → seulement une icône). Tout est dans le jeu : `Ghostship/src/game/camera.c` (11 755 lignes, `gCamera`, `gLakituState`, `update_camera` l.3188, `update_lakitu` l.3047, `update_graph_node_camera` l.3721). La matrice vue est produite dans `geo_process_camera` (`rendering_graph_node.c:343-372`) par `mtxf_lookat(cameraTransform, node->pos, node->focus, node->roll)`, la projection dans `geo_process_perspective` (l.256-290, `node->fov` vient de `geo_camera_fov`). Head-tracking :
- V1 (sans toucher au jeu) : injecter la rotation du casque après la lookat dans `GfxSpMatrix` (point 1b). Le joueur tourne la tête autour du point de vue de Lakitu. Suffisant pour "la tête qui regarde".
- V2 : dans `geo_process_camera`, remplacer `node->focus` par `pos + headForward` et ajouter la translation casque (en unités SM64, ~1 m ≈ 100-150 u) ; neutraliser `node->roll`/`rollScreen` et les secousses (`set_camera_shake_*`) qui sont des sources de nausée. Le FOV du jeu (`node->fov`, 45° par défaut) doit être ignoré au profit du FOV du runtime.

## 4. État de Ghostship

Dernier commit 11 sept 2026 (`develop`). README : ROMs US (SHA-1 9bef1128…) et JP (8a20a5c8…), `.z64` obligatoire ; extraction intégrée au lancement (sélection du ROM → `sm64.o2r`) via le submodule **Torch** (`Torch` à 106621f, `src/port/GameExtractor.cpp`, target CMake `ExtractAssets`). Plateformes : Windows, Linux, macOS, Switch, Android, iOS (dossiers présents). Backends annoncés : DX11 (défaut Windows), OpenGL, Metal, Vulkan. Build Windows (`docs/building.md`) : VS 2022 + v143, CMake, Python ; `cmake -S . -B build/x64 -G "Visual Studio 17 2022" -T v143 -A x64`, dépendances via vcpkg automatisé (`CMakeLists.txt:84` : zlib bzip2 libzip libpng sdl2[vulkan] glew glfw3 nlohmann-json tinyxml2 spdlog mbedtls). Licence MIT (Lywx).

Consommation de libultraship : submodule `libultraship` → `https://github.com/Kenix3/libultraship` épinglé à 93a1a41, intégré par `add_subdirectory(libultraship …)` (`CMakeLists.txt:548-553`). Un libultraship modifié se substitue trivialement : changer l'URL/commit du submodule ou pointer le dossier vers un fork. Attention, Ghostship inclut directement des en-têtes internes (`interpreter.h`, détection `GHOSTSHIP_HAS_CUSTOM_UNIFORMS` l.932) : rester sur la base 93a1a41 pour ne rien casser. Structure : `src/game` (code décompilé SM64), `src/port` (couche port : `Engine.cpp` boucle principale, `interpolation/FrameInterpolation.cpp`, `ShipCompat.cpp`), `src/engine`, `src/port/mods` etc.

## 5. RaYRoD (MVRH / MarioKart64-VR)

- `MarioKart64-VR` : **introuvable/privé**. Impossible de dire où il a mis OpenXR pour MK64.
- `MVRH` main : un README + release de l'exe "hub" (closed source, pas de code). Le README dit : Banjo-Kazooie, Ring Racers, SRB2 ; « standalone Quest versions coming ».
- `MVRH` branche `gpl-source` : sources VR de **SRB2** et **Ring Racers** uniquement (overlay `files/` + `patches/` sur un commit upstream), **GPLv2**. Fichiers : `RingRacers/files/src/vr/vr_openxr.cpp` (5 020 lignes), `vr.h` (API C sans fuite de types XR/GL), `vr_input_bridge.cpp`, `vr_cvars.c` ; `SonicRoboBlast2/files/src/vr/i_openxr.c` (2 331 lignes). Approche lisible dans `vr.h` : `XR_KHR_opengl_enable` + WGL (`vr_openxr.cpp:13,78,2605`), session liée au contexte SDL GL ; **double rendu** par œil (`vr_begin_eye/vr_end_eye`, cible mono MSAA puis blit dans la swapchain) ; projection asymétrique native du runtime (`vr_gl_eye_projection`, commentaire : ne jamais symétriser) ; vue = inverse rigide de la pose tête pré-multipliée au modelview (`vr_gl_eye_view`) ; ciel rendu sans translation (`vr_mono_sky`) ; `XR_KHR_composition_layer_depth` ; HUD = **quad layer head-locked alpha** (`vr_submit_hud_texture`), mode « theater » (frame plate sur grand quad), modes 1re/3e personne/diorama ; manettes exposées comme un **gamepad synthétique** (`VR_GAMEPAD_DEVICE_ID`) + masque `VR_BTN_*` ; monde en « unités par mètre » (`vr_units_per_meter`, défaut 32) ; Android via `xrInitializeLoaderKHR` + GLES. CMake : `find_package(OpenXR CONFIG REQUIRED)` + `OpenXR::openxr_loader opengl32` (patch 002).
- Réutilisable : **l'architecture et les leçons**, pas le code (GPLv2 incompatible avec le MIT de libultraship/Ghostship ; recopier contaminerait le fork). Re-écrire en s'inspirant de l'API de `vr.h`.

## 6. Dépendance OpenXR et choix du backend

Loader : `vcpkg install openxr-loader` s'intègre dans `vcpkg_install_packages(...)` de Ghostship (`CMakeLists.txt:84`) et de `lus-ghostship/cmake/dependencies/windows-vcpkg.cmake:16`, puis `find_package(OpenXR CONFIG)` → `OpenXR::openxr_loader`/`OpenXR::headers`. Alternative sans vcpkg : `FetchContent` de KhronosGroup/OpenXR-SDK (CMake pur, jsoncpp vendu, compile en MSVC sans dépendance externe, cible `openxr_loader`). Les deux sont faciles. Runtime : Oculus/Meta Link expose les deux extensions `XR_KHR_D3D11_enable` et `XR_KHR_opengl_enable`.

Recommandation : **OpenGL** pour la V1.
- Le contexte GL est créé par SDL (`gfx_sdl2.cpp:489`), le binding XR se fait avec `wglGetCurrentDC/Context` sans toucher au backend ; les images de swapchain sont des `GLuint`, blit via `glBlitFramebuffer` depuis `FramebufferOGL::fbo` (`include/fast/backends/gfx_opengl.h:65`). C'est aussi le chemin que RaYRoD a validé sur Link.
- DX11 est le défaut Windows de Ghostship, mais `GfxRenderingAPIDX11` crée son device avec `D3D11CreateDevice(NULL adapter, …)` (`lus-ghostship/src/fast/backends/gfx_direct3d11.cpp:124-126`) : OpenXR exige l'adaptateur LUID de `xrGetD3D11GraphicsRequirementsKHR` → il faut modifier la création du device (chargée dynamiquement, l.204) et faire du `CopyResource`. Faisable en V2, pas prioritaire. Le backend Vulkan est hors scope.

## 7. Architecture recommandée et plan V1

**Choix** : fork de **Kenix3/libultraship@93a1a41** avec un module optionnel `src/xr/` (option CMake `LUS_ENABLE_XR`, define `ENABLE_XR`, no-ops inline sinon, calqué sur le style de `vr.h` de RaYRoD) + fork de Ghostship dont le submodule pointe sur ce fork. Pas d'injection côté jeu : tout ce dont la V1 a besoin passe par `GfxSpMatrix` et `Fast3dWindow`. Le même module servira à SoH/SpaghettiKart s'ils migrent sur cette base (SoH utilise aujourd'hui HM main/SDL3 : ne pas promettre la compatibilité immédiate).

**Étapes concrètes**
1. `src/xr/XrSession.cpp/.h` : instance (`XR_KHR_opengl_enable`), `xrCreateSession` avec `XrGraphicsBindingOpenGLWin32KHR`, espace `LOCAL`, swapchains couleur par œil (`SRGB8_ALPHA8`), `xrWaitFrame/BeginFrame/LocateViews/EndFrame`. Activation par cvar `gXR.Enabled`.
2. `Fast3dWindow::DrawAndRunGraphicsCommands` (`Fast3dWindow.cpp:232`) : si XR actif → `xr->BeginFrame()`, boucle 2 yeux : `interpreter->SetXrEye(eye)`, `StartDrawToFramebuffer(eyeFb[eye])`, `Run(...)`, `CopyFramebuffer(xrFb[eye], eyeFb[eye])` ; puis rendu normal du miroir fenêtre (œil gauche dans `mGfxFrameBuffer` pour `DrawGame`) ; `xr->EndFrame()` juste avant `EndFrame()`. Framebuffers œil créés avec `CreateFrameBuffer` (dimensions = `recommendedImageRectWidth/Height` de `xrEnumerateViewConfigurationViews`), `resize=false`.
3. `Interpreter::GfxSpMatrix` (`lus-ghostship/interpreter.cpp:2288-2292`) : substitution perspective/ortho décrite en 1b ; conserver near/far du jeu ; désactiver `AdjXForAspectRatio` (`interpreter.cpp:1577`) quand XR actif (l'aspect vient du FOV XR).
4. `GfxWindowBackendSDL2::SwapBuffersBegin` : court-circuiter `SyncFramerateWithTime` et `SDL_GL_SetSwapInterval(0)` quand XR actif (c'est `xrWaitFrame` qui cadence).
5. Ghostship : forcer `InterpolationFPS` = refresh casque (`Engine.cpp:1541-1549`, `ProcessGfxCommands` l.1736) ; l'interpolation existante (`FrameInterpolation_Interpolate`) produit alors 72/90 images/s depuis les 30 ticks SM64 — mais chaque sous-frame interpolée devra elle-même être rendue 2 fois.
6. Entrées : V1 = manette classique inchangée ; préparer `PhysicalDeviceType::OpenXR` (point 2).

**Risques**
- ImGui/`DrawGame` suppose une seule texture et calcule letterbox/`mGameWindowViewport` ; le miroir doit rester hors de la boucle œil. `CopyFrameBuffer` (l.5285-5319) tient compte du viewport ImGui : passer par les ids de fb œil explicites.
- `mRendersToFb` peut être faux (résolution = fenêtre, MSAA 1) : forcer `mRendersToFb = true` en XR, sinon `Run` dessine dans le backbuffer.
- HUD/menus 2D et `gSPViewport`/scissor : le HUD ortho projeté sur un plan virtuel est acceptable ; un quad layer séparé (approche RaYRoD) est mieux mais demande un troisième rendu. Les scissors en coordonnées écran (`GfxDpSetScissor`, `CalcAndSetViewport`) doivent être remappés à la taille de l'œil.
- Letterbox N64 320×240 et `AdjXForAspectRatio` : désactiver en XR.
- 30 fps logique : l'interpolation Ghostship atténue, mais les éléments non interpolés (effets, particules, texte) sautilleront ; le pacing `xrWaitFrame` + `SetMaximumFrameLatency(1)` est à vérifier.
- Caméra Lakitu, roll (`rollMtx`, l.357) et secousses : à neutraliser en V2 ; ciel/skybox : rendre sans translation tête (leçon RaYRoD).
- Divergence des forks (Kenix3 vs HM main : CPU vs GPU transform, SDL2 vs SDL3) : le module doit viser le commit épinglé par Ghostship et sera à adapter pour SoH.
- Licences : ne rien copier de MVRH (GPLv2).

À retenir pour la mémoire du projet : Ghostship consomme Kenix3/libultraship@93a1a41 (SDL2, Vulkan, transform GPU par palette), pas HM main ; MarioKart64-VR est privé ; MVRH gpl-source = SRB2/Ring Racers seulement, GPLv2, GL + double rendu + HUD en quad layer.
