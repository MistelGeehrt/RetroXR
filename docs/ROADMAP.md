# Feuille de route

Chaque étape se termine par quelque chose que Guyno peut tester dans le casque. Les cases se cochent au fur et à mesure.

## Étape 0 — Le dépôt (fait)
- [x] Dépôt public, règles, étude technique
- [ ] Renommage `flatverse` → `RetroXR` sur GitHub (à faire par le propriétaire)

## Étape 1 — Mario 64 tourne sur ton PC, sans VR
Objectif : vérifier que la chaîne « récupérer Ghostship + compiler + extraire la ROM » marche de bout en bout.
- [ ] Scripts `bootstrap` et `build` qui produisent un `.exe` sur la CI Windows
- [ ] Tu télécharges l'exe, tu donnes ta ROM, Mario 64 tourne à l'écran
- Ce que tu devras faire : télécharger un fichier, le lancer, choisir ta ROM, me dire si ça marche

## Étape 2 — Mario 64 dans le casque, tête qui regarde
Objectif : la première vraie image VR.
- [ ] Module `xr/` : connexion au casque, une image par œil, projection correcte
- [ ] Suivi de la tête : tu regardes autour, la caméra suit
- [ ] L'interface (pièces, vies, étoiles) posée sur un plan à bonne distance, lisible
- [ ] La fenêtre PC continue d'afficher le jeu (miroir)
- Ce que tu devras faire : brancher le casque, lancer, me dire si l'image est nette, à l'échelle, et si tu as la nausée

## Étape 3 — Confort
- [ ] Suppression des secousses et du roulis de caméra
- [ ] Échelle du monde réglable (Mario à taille réelle ou en « diorama »)
- [ ] Rotation au stick par crans (snap turn) ou fluide, au choix
- [ ] Ciel/horizon stables
- [ ] Profil `profiles/mario64` avec tes réglages

## Étape 4 — Manettes du Quest 3
- [ ] Manettes reconnues comme une manette N64 (stick, A, B, Z, C, Start)
- [ ] Réglage des boutons depuis le menu du jeu
- [ ] Vibration

## Étape 5 — Les autres jeux
- [ ] Mario Kart 64, Star Fox 64, F-Zero X (Ghostship sait déjà les gérer en option)
- [ ] Zelda Ocarina of Time (Ship of Harkinian — base technique un peu différente, à adapter)

## Plus tard
- Réalité mixte Quest 3 (HUD et menus dans le vrai salon)
- Mode « salle de jeu » : l'écran N64 d'origine sur un grand écran virtuel, pour les jeux 2D
- Mélanges de mécaniques entre jeux (le « passthrough » de la vidéo de Léo), rendu possible parce que tout est en code source ouvert sur la même base
