# Contribuer

## Ajouter ou corriger un profil

1. Copier `profiles/_template/profile.json` dans `profiles/<slug-du-jeu>/profile.json` (slug en minuscules, tirets).
2. Remplir au minimum : exécutable, version UE, build UEVR testée, runtime, casque, date, `status`.
3. `python tools/validator/validate_profiles.py` doit passer.
4. Ouvrir une PR avec, dans la description : ce qui marche, ce qui ne marche pas, et si un anti-cheat est présent.
5. Un profil importé d'une autre base garde son auteur dans `provenance.source` et nécessite son accord.

## Scripts Lua

- Préfixe `fv_`, en-tête de licence MIT, `pcall` autour des accès UObject, logs `[flatverse]`.
- Un script générique va dans `lua/lib/`, un script spécifique dans `lua/games/<slug>/`.

## Plugin C++

- Ne jamais inclure autre chose de UEVR que `include/` (MIT). Pas de dépendance au cœur.
- Compiler en Release avant la PR ; la CI Windows doit être verte.

## Commits

Sobres, en français, à l'impératif : `profils: ajoute scorn`, `lua: corrige le crash de fv_dump sur widget détruit`.

## Ce qu'on refuse

Profils pour jeux en ligne ou protégés par anti-cheat ; assets ou binaires de jeu ; binaires UEVR modifiés ; tout ce qui touche à la monétisation de l'accès.
