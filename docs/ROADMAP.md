# Feuille de route (20 semaines)

Chaque étape se termine par un livrable testable en casque. Cocher au fur et à mesure.

## Semaines 1–2 — Prise en main
- [ ] UEVR nightly installée, injection réussie sur 3 jeux de test via Quest Link puis Virtual Desktop
- [ ] `fv_inspect.lua` et `fv_dump.lua` chargés sans erreur dans `log.txt`
- [ ] Plugin C++ compilé (`flatverse.dll`), `flatverse_heartbeat.json` écrit pendant le jeu

## Semaines 3–4 — Dump + schéma
- [ ] `summarize.py` produit un brief lisible pour les 3 jeux
- [ ] `profiles/schema.json` stabilisé (v1), 3 profils écrits à la main et validés par la CI

## Semaines 5–7 — Base de profils v0
- [ ] Manifeste `profiles/index.json` généré par CI et publié en GitHub Release
- [ ] Contact pris avec uevr-profiles.com / Flat2VR ; premier lot importé avec accord
- [ ] Client minimal (Tauri ou .NET) : détecte l'exécutable, télécharge et applique le profil

## Semaines 8–10 — Générateur LLM v0
- [ ] `generate.py` produit un profil `unverified` à partir d'un dump
- [ ] `validator/` étendu : injection, 60 s sans crash, pose caméra suit le casque (lecture du heartbeat)
- [ ] Mesure publiée : taux de profils utilisables sans retouche sur 10 jeux

## Semaines 11–14 — Wrist-HUD
- [ ] Plugin : capture d'un widget UMG dans une texture
- [ ] Plugin : quad rendu dans la pose du contrôleur gauche (VR pure, sans MR)
- [ ] Réglages (main, échelle, offset) persistés dans le profil

## Semaines 15–20 — Réalité mixte expérimentale
- [ ] Prototype chroma-key Virtual Desktop : fond en couleur clé + HUD au poignet dans le salon
- [ ] API layer OpenXR (modèle Rectus) : passthrough FB sur Quest Link en mode développeur
- [ ] Documentation honnête des prérequis runtime pour l'utilisateur final

## Après
- UUVR / Unity avec le même format de profil
- Modding vocal : Whisper local → LLM → script Lua proposé → diff → confirmation (jamais d'exécution directe)

## Jeux de test (solo, sans anti-cheat — vérifier sur PCGamingWiki avant achat)
- UE4 avec profils communautaires : Scorn, Hellblade: Senua's Sacrifice, Stray, Hogwarts Legacy
- UE5 / cas difficiles : Silent Hill 2 Remake (plugin praydog), Hellblade II, Stellar Blade
- À éviter : tout jeu avec EAC/BattlEye actif, tout multijoueur en ligne
