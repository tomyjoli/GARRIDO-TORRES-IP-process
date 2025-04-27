#include <stdio.h>
#include "bmp8.h"


int main() {
    int choix;
    /** Charger l'image
    const char *filename = "barbara_gray.bmp";
    t_bmp8 *img = bmp8_loadImage(filename);
    if (img) {
        // Afficher les informations de l'image
        printf("Image chargée avec succès:\n");
        printf("Largeur: %u\n", img->width);
        printf("Hauteur: %u\n", img->height);
        printf("Profondeur de couleur: %u\n", img->colorDepth);
        printf("Taille des données: %u\n", img->dataSize);

    }
    */

    while (1) {
        printf("Veuillez choisir une option :\n");
        printf("    1. Ouvrir une image\n");
        printf("    2. Sauvegarder une image\n");
        printf("    3. Appliquer un filtre\n");
        printf("    4. Afficher les informations de l'image\n");
        printf("    5. Quitter\n");
        printf(">>> Votre choix : ");

        if (scanf("%d", &choix) != 1) {
            while (getchar() != '\n');
            printf("Entrée invalide. Veuillez réessayer.\n");
            continue;
        }
    }

    switch (choix) {
        case 1:
            printf("Vous avez choisi : %d\n", choix);
            break;
        case 2:
            printf("Vous avez choisi : %d\n", choix);
            break;
        case 3:
            printf("Vous avez choisi : %d\n", choix);
            break;
        case 4:
            printf("Vous avez choisi : %d\n", choix);
            break;
        case 5:
            printf("Au revoir !\n");
            exit(0);
        default:
            printf("Option invalide. Veuillez reessayer.\n");
    }
}