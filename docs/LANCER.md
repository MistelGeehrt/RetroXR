# RetroXR — Mario 64 : lancer le jeu (Windows)

Ce dossier contient le programme compilé par RetroXR. Ne déplacez pas les dossiers `assets/` et le fichier `config.yml` : ils servent à lire votre ROM. Il ne contient AUCUNE donnée Nintendo :
il vous faut votre propre ROM de Super Mario 64 (version US ou JP, fichier .z64).

## Première fois, sans casque (vérification)
1. Mettez votre fichier .z64 dans ce dossier.
2. Lancez RetroXR-Mario64.exe. Le programme détecte la ROM, en extrait les données (quelques secondes), puis le jeu démarre à l'écran.
3. Si le jeu tourne normalement à l'écran, la base est bonne. Passez à la VR.

## En VR (Meta Quest 3)
1. Installez l'application « Meta Quest Link » sur le PC, connectez le casque (câble ou Air Link), et dans ses réglages mettez « OpenXR Runtime » sur Meta Quest Link.
2. Dans le jeu, menu Settings > Graphics : choisissez le backend « OpenGL » (la VR V1 ne marche qu'avec OpenGL), puis relancez.
3. Au lancement, si le casque est détecté, l'image apparaît dedans. Le PC affiche un miroir.
4. Réglages (dans ce dossier, fichier shipofharkinian.json / ghostship.json selon la version, section CVars) :
   - gXR.Enabled : 1 (VR) / 0 (écran)
   - gXR.WorldScale : 1.0 = taille réelle, 0.5 = diorama
   - gXR.UnitsPerMeter : 100 par défaut
   - gXR.HudDistance / gXR.HudHeight : distance et taille de l'interface (pièces, vies)

## Si ça ne marche pas
Envoyez le fichier ghostship.log de ce dossier, et dites : écran noir dans le casque ? image double ? rien dans le casque mais jeu OK à l'écran ?
