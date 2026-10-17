# Écosystème : dépôts et ressources utiles

## Socle

- **UEVR** — https://github.com/praydog/UEVR — injecteur VR Unreal 4.8→5.6. Nightlies : https://github.com/praydog/UEVR-nightly/releases. Site : https://uevr.io. Docs Lua/C++ : https://docs.uevr.io.
- **Forks UE 5.5–5.8** — https://github.com/joeyhodge/UEVR, https://github.com/PureDark/UEVR (correctifs ghosting, jeux récents). À surveiller, pas à forker nous-mêmes.
- **SH2R-UEVR** — https://github.com/praydog/SH2R-UEVR — meilleur exemple de plugin C++ par jeu (désactive la frame gen, force la stéréo native, offsets caméra).
- **uevr-docs** — https://github.com/praydog/uevr-docs (MIT) — source des docs.
- **REFramework** — https://github.com/praydog/REFramework (MIT) — même auteur, RE Engine ; architecture Lua/C++ à étudier.

## Profils et communauté

- **uevr-profiles.com** — base de profils maintenue par Florent (Flat2VR). Premier contact à prendre avant d'importer quoi que ce soit.
- **Flat2VR** (Discord) — communauté UEVR / modding VR. Annonces : https://x.com/Flat2VR.
- **Rai Pal** — https://github.com/Raicuparta/rai-pal — détecte le moteur de toute la bibliothèque Steam, installe UEVR/UUVR. Modèle d'UX pour notre futur client.
- **UEVR Easy Injector** — injecteur automatisé avec profils populaires (communauté).

## Unity (phase 2)

- **UUVR** — https://github.com/Raicuparta/uuvr (GPL-3.0) — stéréo + head-tracking sur Unity. Attention : GPL, à isoler.
- **UnityVRMod** — https://github.com/NewUnityModder/UnityVRMod — alternative BepInEx.

## Réalité mixte

- Passthrough over Quest Link (mode développeur) : https://developers.meta.com/vr/documentation/native/android/mobile-passthrough-over-link/
- **ue-openxr-passthrough** — https://github.com/AgileLens/ue-openxr-passthrough — gestion de l'alpha inversé sur Link (~500 lignes, à lire).
- **openxr-steamvr-passthrough** — https://github.com/Rectus/openxr-steamvr-passthrough — API layer OpenXR, modèle d'architecture pour notre couche MR.
- **WMR-Passthrough** (mbucchia) — https://github.com/mbucchia/WMR-Passthrough — autre référence d'API layer.
- Virtual Desktop : passthrough par chroma-key (couleur unie → caméra). Produits qui s'en servent : Reality Mixer, PassthroughForge.

## IA et rétro-ingénierie

- **decomp.dev** — https://decomp.dev — suivi des décompilations (245+ projets).
- **decomp.wtf** — https://github.com/i12bp8/decomp.wtf — serveur MCP + plugin Claude Code pour la décompilation.
- **ghidra-mcp** — https://github.com/bethington/ghidra-mcp — Ghidra piloté par LLM (209 outils).
- Retour d'expérience : décompilation N64 par Claude en boucle headless (`claude -p` + compilateur comme oracle) — https://simonwillison.net/2025/Dec/6/one-shot-decompilation/

## Hors périmètre mais instructif

- **VorpX** — https://www.vorpx.com — commercial, DX9–12, >250 profils stéréo. Preuve que « générique » = « profils ».
- **VRto3D** — https://github.com/oneup03/VRto3D — driver OpenVR pour écrans 3D, pas un injecteur.
- **R.E.A.L. VR (Luke Ross)** — fermé ; historique DMCA à connaître (voir LEGAL.md).
