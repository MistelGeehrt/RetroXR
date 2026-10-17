# Base de profils

Un dossier par jeu, un `profile.json` par dossier, validé contre `schema.json` par la CI.

- `_template/` : point de départ.
- `status` : `perfect` / `playable` / `needs-work` / `broken` / `unverified`.
- Clé d'identification : nom de l'exécutable (+ sha256 et version quand disponibles), version du moteur, build UEVR testée.
- Un profil généré par IA reste `unverified` jusqu'à un test humain en casque.

Voir CONTRIBUTING.md à la racine.
