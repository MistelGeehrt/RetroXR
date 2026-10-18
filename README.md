# RetroXR

**Jouez aux classiques de la Nintendo 64 en vraie réalité virtuelle sur Meta Quest 3.**
Mario 64 d'abord. Puis Zelda, Mario Kart, Star Fox. Open source, gratuit, avec votre propre cartouche.

> Projet en construction. Rien n'est encore jouable : suivez l'avancement dans [docs/ROADMAP.md](docs/ROADMAP.md).

## L'idée

Les grands jeux N64 ont été intégralement décompilés par la communauté et tournent aujourd'hui nativement sur PC grâce aux ports de [Harbour Masters](https://github.com/HarbourMasters) (Ship of Harkinian pour Zelda, Ghostship pour Mario 64, SpaghettiKart pour Mario Kart…). Tous reposent sur **une même bibliothèque commune**, libultraship.

RetroXR ajoute la réalité virtuelle **dans cette bibliothèque commune**, pas dans chaque jeu : le travail fait pour Mario 64 sert aux autres.

Ce que ça donne dans le casque :
- le monde en vraie 3D stéréoscopique, à l'échelle,
- la tête qui regarde librement autour de Mario,
- les manettes du Quest 3 à la place de la manette N64, avec des réglages par jeu,
- un confort travaillé (pas de secousses de caméra, interface lisible, vitesse de rotation réglable).

## Ce qu'il faut

- Un PC Windows avec une carte graphique récente (NVIDIA GTX 1060 / RTX ou AMD équivalent).
- Un Meta Quest 3 relié au PC par câble (Quest Link) ou par Wi-Fi (Air Link / Virtual Desktop).
- **Votre propre ROM** du jeu, au format `.z64`. RetroXR ne contient aucun contenu Nintendo et n'en fournira jamais.

## Comment ça marche (version simple)

RetroXR ne réécrit pas les jeux. Il prend le port PC officiel de la communauté (Ghostship pour Mario 64), y ajoute un module VR, et vous donne un programme à lancer. Au premier lancement, le programme vous demande votre ROM, en extrait les données, puis vous pouvez jouer.

## Ce dépôt contient

| Dossier | Rôle |
|---|---|
| `xr/` | Le module VR (notre code) : liaison OpenXR, rendu pour chaque œil, suivi de la tête, manettes |
| `patches/` | Les petites modifications à appliquer à libultraship et à Ghostship pour y brancher `xr/` |
| `profiles/` | Réglages de confort par jeu (échelle, rotation, boutons) |
| `scripts/` | Scripts qui récupèrent les projets d'origine, appliquent les patches et compilent |
| `docs/` | Feuille de route, architecture, étude technique, cadre juridique |

## Règles du projet

- **Aucune ROM, aucun asset, aucune clé** dans ce dépôt, jamais. Même règle que les ports Harbour Masters.
- **Gratuit, sans paywall.** Dons bienvenus, accès jamais vendu.
- Jeux solo uniquement. Pas de triche, pas de multijoueur en ligne concerné.
- Le code est écrit en grande partie avec des agents IA et relu, testé, validé en casque par des humains. Voir [AI_POLICY.md](AI_POLICY.md).

## Remerciements

Harbour Masters (libultraship, Ghostship, Ship of Harkinian), l'équipe de décompilation de Super Mario 64, le projet Torch, et RaYRoD dont les ports VR ont montré la voie.

RetroXR n'est affilié ni à Nintendo, ni à Meta, ni à Harbour Masters.

## Licence

Code : MIT. Documentation et profils : CC-BY-4.0. Les projets d'origine gardent leurs propres licences (MIT pour libultraship et Ghostship).
