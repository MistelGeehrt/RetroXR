# Contribuer

## Tester (le plus utile)
Téléchargez le dernier artefact `RetroXR-Mario64-windows` dans l'onglet Actions, suivez `LISEZMOI.md`, et ouvrez une issue avec : votre carte graphique, le runtime (Quest Link / Virtual Desktop / Steam Link), le fichier `ghostship.log`, et ce que vous voyez dans le casque.

## Code
- Le module VR est dans `xr/` (C++17, API C dans `xr.h`). Il ne dépend que d'OpenXR et d'OpenGL.
- Les modifications aux projets d'origine sont des patches dans `patches/`, générés avec `git diff` sur les commits de `scripts/pins.json`. Ils doivent passer `git apply --check` (la CI le vérifie).
- Pour travailler en local : `scripts\bootstrap.ps1` puis `scripts\build.ps1` (Windows, Visual Studio 2022, CMake, Python).
- Commits sobres, en français, à l'impératif.

## Ce qu'on refuse
ROM, assets ou clés ; liens vers des ROM ; code sous GPL ; tout ce qui touche à la monétisation de l'accès.
