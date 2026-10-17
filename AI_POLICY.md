# Politique d'usage de l'IA

Ce projet est développé en grande partie avec des agents de code (Claude Code et autres). On le dit, et on encadre.

1. **L'IA produit des artefacts, pas des vérités.** Un profil ou un script généré est marqué `generated_by_ai: true` et `status: unverified` tant qu'un humain ne l'a pas testé en casque.
2. **Jamais dans la boucle temps réel.** Aucun appel à un modèle pendant le rendu ou le tick du jeu.
3. **Toute contribution générée doit passer les mêmes tests** que le code écrit à la main (CI, validation de schéma, test en jeu documenté dans la PR).
4. **Pas de copie de code sous licence incompatible** via l'IA : un agent qui « se souvient » du cœur d'UEVR (non permissif) ou d'un mod fermé ne doit pas le recracher ici. En cas de doute, on réécrit.
5. **Les contributeurs qui refusent l'IA sont les bienvenus.** Indiquer `[humain]` dans le titre de la PR suffit ; personne n'est obligé d'en utiliser.
6. Les prompts système des outils (`tools/profile-gen/prompt.md`) sont versionnés : changer le comportement de l'IA passe par une PR comme tout le reste.
