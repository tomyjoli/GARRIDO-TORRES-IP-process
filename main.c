#include <stdio.h>
#include <stdlib.h>
#include "bmp8.h"
#include " bmp8.c"


void appliquer_filtre(t_bmp8 *img);

int main() {
    t_bmp8 *img = NULL;
    int choix;

    while (1) {
        printf("\n========== Menu principal ==========\n");
        printf("1. Ouvrir une image\n");
        printf("2. Appliquer un filtre\n");
        printf("3. Sauvegarder l'image\n");
        printf("4. Afficher les informations de l'image\n");
        printf("5. Quitter\n");
        printf(">>> Votre choix : ");

        if (scanf("%d", &choix) != 1) {
            while (getchar() != '\n');
            printf("Entrée invalide. Réessayez.\n");
            continue;
        }

        if (choix == 5) {
            printf("Fin du programme.\n");
            break;
        }

        switch (choix) {
            case 1: {
                char chemin[256];
                printf("Chemin de l'image : ");
                scanf("%s", chemin);
                if (img) bmp8_free(img); // Libérer ancienne image
                img = bmp8_loadImage(chemin);
                if (img) printf("Image chargée avec succès !\n");
                break;
            }
            case 2:
                if (img) {
                    appliquer_filtre(img);
                } else {
                    printf("Veuillez ouvrir une image d'abord.\n");
                }
                break;
            case 3:
                if (img) {
                    char chemin[256];
                    printf("Chemin pour sauvegarder l'image : ");
                    scanf("%s", chemin);
                    bmp8_saveImage(chemin, img);
                    printf("Image sauvegardée !\n");
                } else {
                    printf("Veuillez ouvrir une image d'abord.\n");
                }
                break;
            case 4:
                if (img) {
                    bmp8_printInfo(img);
                } else {
                    printf("Veuillez ouvrir une image d'abord.\n");
                }
                break;
            default:
                printf("Option invalide. Réessayez.\n");
        }
    }

    if (img) bmp8_free(img);
    return 0;
}

void appliquer_filtre(t_bmp8 *img) {
    int choix_filtre;
    while (1) {
        printf("\n--- Menu des filtres ---\n");
        printf("1. Appliquer un négatif\n");
        printf("2. Modifier la luminosité\n");
        printf("3. Binariser (seuillage)\n");
        printf("4. Retour au menu principal\n");
        printf(">>> Votre choix : ");

        if (scanf("%d", &choix_filtre) != 1) {
            while (getchar() != '\n');
            printf("Entrée invalide. Réessayez.\n");
            continue;
        }

        switch (choix_filtre) {
            case 1:
                bmp8_negative(img);
                printf("Négatif appliqué !\n");
                break;
            case 2: {
                int valeur;
                printf("Valeur de luminosité (+/-) : ");
                scanf("%d", &valeur);
                bmp8_brightness(img, valeur);
                printf("Luminosité ajustée !\n");
                break;
            }
            case 3: {
                int seuil;
                printf("Seuil (0-255) : ");
                scanf("%d", &seuil);
                bmp8_threshold(img, seuil);
                printf("Binarisation effectuée !\n");
                break;
            }
            case 4:
                printf("Retour au menu principal.\n");
                return;
            default:
                printf("Option invalide. Réessayez.\n");
        }
    }
}