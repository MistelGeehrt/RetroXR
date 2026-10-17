# Arbitrage d'architecture (octobre 2026)

Synthèse de la recherche menée avant la création du dépôt. Les sources sont dans [ECOSYSTEM.md](ECOSYSTEM.md).

## Les trois options

| Critère | (A) Injecteur DX/Vulkan générique | (B) Injecteur Unreal refait de zéro | **(C) Extensions UEVR** |
|---|---|---|---|
| Caméra / 6DoF | Chasse aux matrices jeu par jeu | Réflexion moteur à redécouvrir | Déjà résolu |
| Stéréo | Profondeur (artefacts) ou double rendu par jeu | Pipeline natif à réécrire | Native Stereo existant |
| HUD | Heuristique fragile sur draw calls | Widgets UMG identifiables | Projeté + scriptable en Lua |
| Premier jeu jouable | Mois | 6–12 mois | Jours |
| Différenciation | Nulle en pratique (VorpX = profils) | Nulle (UEVR existe) | Forte : profils, IA, MR |
| Compatible vibe coding | Non | Non | Oui |

**Décision : C.** Deux couches : in-process (plugin C++ + Lua) et hors-process (outillage Python, base de profils, plus tard un API layer OpenXR pour la réalité mixte).

## Priorité des idées initiales

1. **Base de profils structurée et versionnée.** L'offre existante (Discord Flat2VR, uevr-profiles.com, tableurs) est dispersée, non versionnée et non validée. Clé = exécutable + version du jeu + build UEVR testée. À fédérer avec les auteurs existants, pas à concurrencer.
2. **Générateur de profils assisté par LLM.** Le LLM lit un dump UObject et propose profil + scripts Lua ; un validateur déterministe teste ; l'humain tranche. Modèle calqué sur ce qui marche en décompilation assistée (oracle = compilateur ; ici oracle = smoke test en jeu).
3. **Wrist-HUD puis HUD en réalité mixte.** D'abord un quad au poignet en VR pure (utile tout de suite). La MR dépend du runtime : chroma-key Virtual Desktop (zéro code), ou API layer OpenXR sur Quest Link en mode développeur. Steam Link n'expose pas le passthrough.
4. **Modding vocal.** Gadget, en dernier, et jamais d'exécution directe : Whisper → LLM → script Lua proposé → diff → confirmation.

## Pourquoi l'« agent IA qui lit la RAM » est abandonné

Un LLM met des secondes par appel ; une frame dure 11 ms. Il n'a pas d'oracle pour distinguer la matrice de la caméra de gameplay de celle des ombres. Et sur Unreal, la caméra n'est pas à trouver : la réflexion donne `PlayerCameraManager`. L'IA sert à écrire le scanner et le profil, pas à scanner.

## Licence d'UEVR (vérifié sur le dépôt)

`LICENSE` racine : « Copyright (c) 2022-2025 praydog. All rights reserved. » — non permissif. `include/LICENSE` et `examples/LICENSE` : MIT, « separate from the license for the rest of the UEVR codebase ». Conséquence : on embarque l'API, jamais le cœur ni un binaire modifié.
