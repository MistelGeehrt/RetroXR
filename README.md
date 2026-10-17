# flatverse

Extensions open source pour [UEVR](https://github.com/praydog/UEVR), pensées pour le Meta Quest 3 : une base de profils structurée, un générateur de profils assisté par IA, et un HUD au poignet (puis en réalité mixte).

> Nom de travail. UEVR est l'œuvre de praydog ; flatverse n'est ni affilié ni une redistribution d'UEVR. Il faut installer UEVR séparément.

## Pourquoi pas un injecteur VR « universel » ?

Parce que ça n'existe pas, et pas faute d'essais. VorpX y travaille depuis plus de dix ans et reste une collection de profils par jeu. UEVR résout caméra, stéréo native et HUD pour tout Unreal Engine 4.8 → 5.6 grâce à la réflexion UObject. La valeur se trouve au-dessus, pas en dessous. Détails dans [docs/ARBITRAGE.md](docs/ARBITRAGE.md).

## Ce que contient ce dépôt

| Dossier | Contenu | Licence |
|---|---|---|
| `plugin/` | Plugin C++ UEVR (`flatverse.dll`) : télémétrie VR → JSON, réception d'événements Lua, futur rendu du wrist-HUD | MIT |
| `plugin/external/uevr-api/` | Copie de `include/uevr` d'UEVR | MIT (praydog) |
| `lua/lib/` | Scripts UEVR réutilisables : `fv_inspect`, `fv_dump`, `fv_wrist_hud` | MIT |
| `lua/games/<slug>/` | Scripts spécifiques à un jeu | MIT |
| `profiles/` | Base de profils (`schema.json` + un dossier par jeu) | CC-BY-4.0 |
| `tools/uobject-dump/` | Résumé d'un dump UObject | MIT |
| `tools/profile-gen/` | Génération de profil via Claude Code (`claude -p`) | MIT |
| `tools/validator/` | Validation des profils (CI) | MIT |
| `docs/` | Arbitrage, feuille de route, écosystème, légal | CC-BY-4.0 |

## Démarrage rapide (Windows, Quest 3)

1. Installer UEVR (nightly recommandée) et un runtime : Virtual Desktop (VDXR) ou Quest Link (OpenXR Meta).
2. Lancer un jeu Unreal **solo, hors ligne, sans anti-cheat** et l'injecter avec UEVR.
3. Copier `lua/lib/fv_inspect.lua` et `lua/lib/fv_dump.lua` dans `%APPDATA%\UnrealVRMod\<NomDuJeu>\scripts\`.
4. En jeu, ouvrir l'overlay UEVR (touche Insert) : le panneau `flatverse / inspect` montre les poses ; `Dump UObjects` écrit `scripts/data/flatverse_dump.json`.
5. Côté PC :
   ```
   pip install jsonschema
   python tools/uobject-dump/summarize.py "%APPDATA%\UnrealVRMod\<NomDuJeu>\scripts\data\flatverse_dump.json"
   python tools/profile-gen/generate.py <dump> --title "Nom" --exe "Nom-Win64-Shipping.exe" --out profiles/nom/profile.json
   python tools/validator/validate_profiles.py
   ```
6. Plugin C++ (optionnel pour l'instant) : `cmake -S plugin -B build -G "Visual Studio 17 2022" -A x64 && cmake --build build --config Release`, puis copier `flatverse.dll` dans `%APPDATA%\UnrealVRMod\<NomDuJeu>\plugins\`.

## Règles non négociables

- **Jamais de jeu en ligne ni d'anti-cheat** (EAC, BattlEye, Vanguard, Ricochet). L'injection est indiscernable d'un cheat pour ces systèmes : ban garanti.
- **Jamais de paywall.** Les retraits DMCA récents (Take-Two 2022, CD Projekt et 505 Games 2026) visaient des mods payants. Dons acceptés, accès jamais vendu.
- **Jamais d'assets, de clés ni de binaires UEVR modifiés** dans ce dépôt.
- **L'IA propose, elle ne valide pas.** Tout profil généré est `unverified` tant qu'un humain ne l'a pas testé en casque. Voir [AI_POLICY.md](AI_POLICY.md).

## Feuille de route

Voir [docs/ROADMAP.md](docs/ROADMAP.md) : 20 semaines, du premier script Lua au prototype de HUD en réalité mixte.

## Contribuer

[CONTRIBUTING.md](CONTRIBUTING.md). Les profils existants de la communauté (uevr-profiles.com, Flat2VR) ne sont importés qu'avec l'accord de leurs auteurs.
