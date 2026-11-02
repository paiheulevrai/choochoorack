# Les Forges du Brouillard

[Ouvrir le patch](Les_Forges_du_Brouillard.vcv) dans VCV Rack 2.

Une installation ambient industrielle autogénérative à **104 BPM** : nappes
profondes, sous-basse, métal résonant, frappes massives et poussières stéréo.
Le repère esthétique est [VHIKK X](https://forge-tme.com/vhikk-x/) : une matière
qui se transforme lentement, avec des zones plus instables à explorer en live.

**Tous les sons viennent de ChooChooRack** : trois MME, trois Bogie et trois
Sintered. Les autres modules assurent uniquement le contrôle, le filtrage,
le mixage et les effets. Deux MME jouent quatre voix chacun ; Comb joue trois
voix avec des déclenchements indépendants.

## Ouvrir

Modules nécessaires, déjà installés sur cette machine :

- ChooChooRack 2.0.2 ou version ultérieure.
- VCV Fundamental 2.6.4.
- Valley 2.4.5, pour Plateau.

Le patch démarre seul. La sortie Audio 2 est réglée sur les haut-parleurs Realtek
de cette machine, à 48 kHz. Choisir une autre sortie dans Audio 2 si nécessaire.
Le tempo affiché sur le premier SEQ3 est **208 BPM**, car il fournit les croches ;
la pulsation musicale est à 104 BPM.

## Jouer l'installation

Le premier **8vert de la quatrième rangée**, juste après le panneau de notes,
regroupe quatre gestes. Ses quatre premiers potentiomètres sont à utiliser
dans leur moitié positive :

| Ligne | Geste | Effet |
| --- | --- | --- |
| 1 | Énergie | Niveau des deux groupes de percussions ; à zéro, les nappes restent. |
| 2 | Profondeur | Ouvre progressivement les filtres des nappes. |
| 3 | Métal | Augmente la modulation de Sintered et le feedback des deux nappes MME. |
| 4 | Espace | Dose la réverbération stéréo. |

Dans la même rangée, le premier VCV VCMixer dose kick, snare et métal Bogie ;
le deuxième dose Comb, Shard et Squall. Le VCMixer de la deuxième rangée dose
nappe profonde, reflet spectral et sous-basse.

Les mouvements lents sont les plus efficaces. **Hold** sur Plateau suspend le
paysage ; le relâcher pour laisser revenir les percussions et les traînes.
Les deux delays croisés permettent de resserrer l'espace ou d'étirer les échos.

Les cycles de huit et sept pas se décalent. Les accords changent environ toutes
les neuf secondes, tandis que les probabilités, les formes et les filtres
évoluent à des vitesses différentes. Laisser tourner plusieurs minutes.

Les couleurs de câbles distinguent audio (rose), pitch (bleu), rythme (orange)
et modulation (vert).
