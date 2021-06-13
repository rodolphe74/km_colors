# km_colors
Display jpeg colors inside a rotating cube.
Use [k-mean](https://github.com/ogus/kmeans-quantizer) algorithm to find a 8 colors optimal palette.

![teojpg](/samples/sample.gif)

# Que dire de plus ?
- Il s'agit d'un programme en C utilisant la librairie SDL pour l'affichage et l'animation.
- Les couleurs sont affichées en nuage de points à l'intérieur d'un cube. La coordonnée 3D d'un point correspond a la nuance (sur 256) de chaque composante RVB d'une couleur.
- Les 8 couleurs resultantes de la quantification sont encerclées.
- Le programme prend 2 arguments:
  - Le chemin de la jpeg
  - Le seuil de fréquence d'une couleur en dessous de laquelle la couleur n'est pas affichée dans le cube (par défaut 20).
- La projection 3D est faite à l'ancienne (pas d'accéleration matériel) - Ce [site](https://www.scratchapixel.com/index.php) est une mine d'or pour comprendre comment faire de la 3D, en particulier ce [chapitre](https://www.scratchapixel.com/lessons/3d-basic-rendering/computing-pixel-coordinates-of-3d-point/mathematics-computing-2d-coordinates-of-3d-points).
