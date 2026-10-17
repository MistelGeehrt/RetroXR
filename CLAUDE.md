# CLAUDE.md — contexte pour Claude Code

Ce fichier est lu automatiquement par Claude Code. Il résume ce qu'il faut savoir avant de toucher au dépôt.

## Le projet en trois lignes

flatverse = extensions au-dessus d'UEVR (injecteur VR pour jeux Unreal Engine, par praydog). On ne réécrit PAS d'injecteur, on ne hooke PAS DirectX, on ne scanne PAS la mémoire à la main. Tout passe par l'API UEVR (C++ dans `plugin/external/uevr-api/uevr/`, Lua via `uevr.*`).

## Décisions figées (ne pas rouvrir sans l'accord de Guyno)

- Option C de `docs/ARBITRAGE.md` : plugin C++ + Lua + outillage hors-process. Pas d'injecteur générique.
- L'IA produit des artefacts (profil, script Lua) validés de façon déterministe. Jamais de LLM dans la boucle de rendu.
- Cibles de test : jeux solo hors ligne sans anti-cheat uniquement.
- Pas de paywall, pas d'assets de jeu, pas de binaire UEVR dans le dépôt.
- Licences : MIT pour le code, CC-BY-4.0 pour `profiles/` et `docs/`.

## Références d'API à consulter avant d'écrire du code

- C++ : `plugin/external/uevr-api/uevr/Plugin.hpp` (callbacks `on_*`), `API.hpp` (`API::VR::get_pose`, `find_uobject`, `get_persistent_dir`, `UObjectHook`). Exemple complet : `docs/uevr_example_plugin.cpp.txt`.
- Lua : docs officielles https://docs.uevr.io (pages `plugins/lua/*`). Bindings vérifiés : `uevr.api:find_uobject / get_local_pawn / get_player_controller / execute_command / dispatch_custom_event`, `uevr.params.vr:get_pose / get_hmd_index / get_left_controller_index / is_openxr`, `UEVR_UObjectHook.get_objects_by_class(klass, false)`, `UStruct:get_child_properties / get_super`, `FProperty:get_offset / get_next`, `imgui.text / button / checkbox / same_line / collapsing_header / slider_float`, `json.dump_file / load_file`, callbacks `on_pre_engine_tick / on_post_engine_tick / on_draw_ui / on_script_reset / on_xinput_get_state / on_lua_event`.
- `find_uobject` est coûteux : le mettre en cache, jamais dans une boucle par frame.
- Scripts Lua : `%APPDATA%\UnrealVRMod\<Jeu>\scripts\`, plugins : `...\plugins\`. Les chemins `json.dump_file` sont relatifs au dossier `scripts/`.

## Conventions

- Préfixe `fv_` pour les scripts Lua de `lua/lib/`.
- Toujours `pcall` autour des accès UObject : un objet peut être détruit entre deux ticks.
- Logs : `print("[flatverse] ...")` en Lua, `API::get()->log_info("[flatverse] ...")` en C++. En vibe coding, le log est la seule source de vérité quand le jeu plante.
- Un profil = `profiles/<slug>/profile.json`, validé par `python tools/validator/validate_profiles.py`.
- Commits sobres, en français, sans ligne d'attribution.

## Ce qu'il faut faire quand un jeu plante avec un script

1. Récupérer `%APPDATA%\UnrealVRMod\<Jeu>\log.txt` et la sortie `[flatverse]`.
2. Reproduire avec `fv_inspect.lua` seul : si ça plante encore, c'est UEVR ou le jeu, pas nous.
3. Réduire le script jusqu'à la ligne fautive avant de proposer un correctif.

## Build du plugin

```
cmake -S plugin -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Dépendances : Visual Studio 2022 (C++23), CMake ≥ 3.21. Pas d'ImGui pour l'instant (volontaire).
