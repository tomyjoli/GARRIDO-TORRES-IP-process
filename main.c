#include <stdio.h>
#include <stdlib.h>

#include "bmp8.h"
#include "bmp24.h"

// Prototypes pour les fonctions de menu des filtres
void menu_appliquer_filtre_bmp8(t_bmp8 *img);
void menu_appliquer_filtre_bmp24(t_bmp24 *img);

// Fonction pour vider le buffer d'entrée jusqu'au prochain '\n' ou EOF
void vider_buffer_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Fonction principale du programme
// Gère le menu principal, le chargement/sauvegarde d'images et l'application des filtres
int main() {
    t_bmp8 *img8 = NULL;
    t_bmp24 *img24 = NULL;
    int current_image_type = 0;
    int choix;
    char chemin[FILENAME_MAX];

    // Message de bienvenue
    printf("Bienvenue dans le programme de Traitement d'Images en C !\n");
    printf("Projet TI202 - EFREI Paris \n");

    // Boucle principale du programme
    while (1) {
        // Affichage du menu principal
        printf("\n========== Menu principal ==========\n");

        // Affiche l'image actuellement chargée
        if (current_image_type == 8 && img8) {
            printf("Image 8-bits chargée (W:%u, H:%u).\n", img8->width, img8->height);
        }
        else if (current_image_type == 24 && img24) {
            printf("Image 24-bits chargée (W:%d, H:%d).\n", img24->info_header.width, abs(img24->info_header.height));
        }
        else {
            printf("Aucune image chargée.\n");
        }

        // Options du menu principal
        printf("-----------------------------------\n");
        printf("1. Ouvrir une image 8-bits (N&B)\n");
        printf("2. Ouvrir une image 24-bits (Couleur)\n");
        printf("3. Appliquer un filtre\n");
        printf("4. Sauvegarder l'image\n");
        printf("5. Afficher les informations de l'image\n");

        // Options spécifiques selon le type d'image chargée (8-bits ou 24-bits)
        if (current_image_type == 8 && img8) {
            printf("--- Options BMP 8-bits ---\n");
            printf("  7. Afficher l'histogramme (8-bits)\n");
            printf("  8. Égaliser l'histogramme (8-bits)\n");
        }
        if (current_image_type == 24 && img24) {
            printf("--- Options BMP 24-bits ---\n");
            printf("  9. Égaliser l'histogramme couleur (YUV)\n");
        }
        printf("-----------------------------------\n");
        printf("10. Quitter\n");
        printf(">>> Votre choix : ");

        // Gestion de l'entrée utilisateur
        if (scanf("%d", &choix) != 1) {
            printf("Entrée invalide. Veuillez entrer un nombre.\n");
            vider_buffer_stdin();
            continue;
        }
        vider_buffer_stdin();

        // fin du programme
        if (choix == 10) {
            printf("Fin du programme.\n");
            break;
        }

        // Traitement des choix de menu
        switch (choix) {
            case 1: // Ouvrir BMP8
                printf("Chemin de l'image 8-bits : ");
                if (scanf("%s", chemin) == 1) {
                    // vider_buffer_stdin(); // Moins critique après %s, mais peut l'être si entrée très longue
                    if (img8) bmp8_free(img8); img8 = NULL;
                    if (img24) bmp24_free(img24); img24 = NULL;
                    img8 = bmp8_loadImage(chemin);
                    if (img8) {
                        printf("Image 8-bits '%s' chargée avec succès !\n", chemin);
                        current_image_type = 8;
                    }
                    else current_image_type = 0;
                }
                else {
                    printf("Erreur de lecture du chemin.\n");
                    vider_buffer_stdin();
                }
                break;

            case 2: // Ouvrir BMP24
                printf("Chemin de l'image 24-bits : ");
                if (scanf("%s", chemin) == 1) {
                    // vider_buffer_stdin();
                    if (img8) bmp8_free(img8); img8 = NULL;
                    if (img24) bmp24_free(img24); img24 = NULL;
                    img24 = bmp24_loadImage(chemin);
                    if (img24) {
                        printf("Image 24-bits '%s' chargée avec succès !\n", chemin);
                        current_image_type = 24;
                    }
                    else current_image_type = 0;
                }
                else {
                    printf("Erreur de lecture du chemin.\n");
                    vider_buffer_stdin();
                }
                break;

            case 3: // Appliquer filtre
                if (current_image_type == 8 && img8) {
                    menu_appliquer_filtre_bmp8(img8);
                }
                else if (current_image_type == 24 && img24) {
                    menu_appliquer_filtre_bmp24(img24);
                }
                else printf("Veuillez ouvrir une image d'abord.\n");
                break;

            case 4: // Sauvegarder
                if (current_image_type == 0) {
                    printf("Aucune image chargée à sauvegarder.\n");
                    break;
                }
                printf("Chemin pour sauvegarder l'image : ");
                if (scanf("%s", chemin) == 1) {
                    if (chemin[0] == '\0') { // Vérifier si la chaîne est vide
                        printf("Chemin de sauvegarde vide. Opération annulée.\n");
                        break;
                    }
                    if (current_image_type == 8 && img8) bmp8_saveImage(chemin, img8);
                    else if (current_image_type == 24 && img24) bmp24_saveImage(chemin, img24);
                }
                else {
                    printf("Erreur de lecture du chemin pour la sauvegarde.\n");
                    vider_buffer_stdin();
                }
                break;

            case 5: // Infos
                if (current_image_type == 8 && img8) bmp8_printInfo(img8);
                else if (current_image_type == 24 && img24) bmp24_printInfo(img24);
                else printf("Veuillez ouvrir une image d'abord.\n");
                break;

            case 7: // Afficher histogramme 8-bits
                if (current_image_type == 8 && img8) bmp8_printHistogram(img8);
                else printf("Option disponible uniquement pour une image 8-bits chargée.\n");
                break;

            case 8: // Égaliser histogramme 8-bits
                if (current_image_type == 8 && img8) {
                    bmp8_equalizeHistogram(img8);
                }
                else printf("Option disponible uniquement pour une image 8-bits chargée.\n");
                break;

            case 9: // Égaliser histogramme 24-bits (YUV)
                if (current_image_type == 24 && img24) {
                    bmp24_equalize(img24);
                }
                else {
                    printf("Option disponible uniquement pour une image 24-bits chargée.\n");
                }
                break;

            default:
                printf("Option invalide (%d). Réessayez.\n", choix);
        }
    }

    if (img8) bmp8_free(img8);
    if (img24) bmp24_free(img24);

    printf("Nettoyage et fin.\n");
    return 0;
}

void menu_appliquer_filtre_bmp8(t_bmp8 *img) {
    int choix_filtre;
    int valeur_param;

    // Vérification de l'image
    if (!img) {
        printf("Erreur : Aucune image 8-bits pour appliquer un filtre.\n");
        return;
    }

    // Boucle du menu des filtres 8-bits
    while(1) {
        printf("\n--- Menu des filtres 8-bits ---\n");
        printf("1. Négatif\n");
        printf("2. Modifier la luminosité\n");
        printf("3. Binariser (seuillage)\n");
        printf("4. Flou (Box Blur)\n");
        printf("5. Flou gaussien\n");
        printf("6. Contours (Outline)\n");
        printf("7. Relief (Emboss)\n");
        printf("8. Netteté (Sharpen)\n");
        printf("9. Retour au menu principal\n");
        printf(">>> Votre choix : ");

        // Gestion de l'entrée utilisateur
        if (scanf("%d", &choix_filtre) != 1) {
            printf("Entrée de filtre invalide. Veuillez entrer un nombre.\n");
            vider_buffer_stdin();
            continue;
        }
        vider_buffer_stdin();

        //Retour au menu principal
        if (choix_filtre == 9) {
            printf("Retour au menu principal.\n");
            return;
        }

        // Application du filtre sélectionné
        switch (choix_filtre) {
            case 1:
                bmp8_negative(img);
                printf("Négatif (8-bits) appliqué !\n");
                break;
            case 2:
                printf("Valeur de luminosité (-255 à 255) : ");
                if (scanf("%d", &valeur_param) == 1) {
                    vider_buffer_stdin();
                    bmp8_brightness(img, valeur_param);
                    printf("Luminosité (8-bits) ajustée !\n");
                }
                else {
                    printf("Valeur de luminosité invalide.\n");
                    vider_buffer_stdin();
                }
                break;
            case 3:
                printf("Seuil (0 à 255) : ");
                if (scanf("%d", &valeur_param) == 1) {
                    vider_buffer_stdin();
                    bmp8_threshold(img, valeur_param);
                    printf("Binarisation (8-bits) effectuée !\n");
                }
                else {
                    printf("Valeur de seuil invalide.\n");
                    vider_buffer_stdin();
                }
                break;
            case 4: bmp8_boxBlur(img); printf("Flou (Box Blur 8-bits) appliqué !\n"); break;
            case 5: bmp8_gaussianBlur(img); printf("Flou Gaussien (8-bits) appliqué !\n"); break;
            case 6: bmp8_outline(img); printf("Contours (Outline 8-bits) appliqués !\n"); break;
            case 7: bmp8_emboss(img); printf("Relief (Emboss 8-bits) appliqué !\n"); break;
            case 8: bmp8_sharpen(img); printf("Netteté (Sharpen 8-bits) appliquée !\n"); break;
            default:
                printf("Option de filtre (8-bits) invalide. Réessayez.\n");
        }
    }
}

void menu_appliquer_filtre_bmp24(t_bmp24 *img) {
    int choix_filtre;
    int valeur_param;

    // Vérification des filtres
    if (!img) {
        printf("Erreur : Aucune image 24-bits pour appliquer un filtre.\n");
        return;
    }

    // Boucle du menu des filtres 24-bits
    while(1) {
        printf("\n--- Menu des filtres 24-bits ---\n");
        printf("1. Négatif\n");
        printf("2. Niveaux de gris\n");
        printf("3. Modifier la luminosité\n");
        printf("4. Binariser (seuillage à partir du gris)\n");
        printf("5. Flou (Box Blur)\n");
        printf("6. Flou gaussien\n");
        printf("7. Contours (Outline)\n");
        printf("8. Relief (Emboss)\n");
        printf("9. Netteté (Sharpen)\n");
        printf("10. Retour au menu principal\n");
        printf(">>> Votre choix : ");

        // Gestion de l'entrée utilisateur
        if (scanf("%d", &choix_filtre) != 1) {
            printf("Entrée de filtre invalide. Veuillez entrer un nombre.\n");
            vider_buffer_stdin();
            continue;
        }
        vider_buffer_stdin();

        //Retour au menu principal
        if (choix_filtre == 10) {
            printf("Retour au menu principal.\n");
            return;
        }

        // Application du filtre sélectionné
        switch (choix_filtre) {
            case 1:
                bmp24_negative(img);
                printf("Négatif (24-bits) appliqué !\n");
                break;
            case 2:
                bmp24_grayscale(img);
                printf("Niveaux de gris (24-bits) appliqués !\n");
                break;
            case 3:
                printf("Valeur de luminosité (-255 à 255) : ");
                if (scanf("%d", &valeur_param) == 1) {
                    vider_buffer_stdin();
                    bmp24_brightness(img, valeur_param);
                    printf("Luminosité (24-bits) ajustée !\n");
                }
                else {
                    printf("Valeur de luminosité invalide.\n");
                    vider_buffer_stdin();
                }
                break;
            case 4: // bmp24_threshold
                printf("Seuil (0 à 255) : ");
                if (scanf("%d", &valeur_param) == 1) {
                    vider_buffer_stdin();
                    bmp24_threshold(img, valeur_param);
                    printf("Seuillage (24-bits) appliqué !\n");
                }
                else {
                    printf("Valeur de seuil invalide.\n");
                    vider_buffer_stdin();
                }
                break;
            case 5: bmp24_boxBlur(img); printf("Flou (Box Blur 24-bits) appliqué !\n"); break;
            case 6: bmp24_gaussianBlur(img); printf("Flou Gaussien (24-bits) appliqué !\n"); break;
            case 7: bmp24_outline(img); printf("Contours (Outline 24-bits) appliqués !\n"); break;
            case 8: bmp24_emboss(img); printf("Relief (Emboss 24-bits) appliqué !\n"); break;
            case 9: bmp24_sharpen(img); printf("Netteté (Sharpen 24-bits) appliquée !\n"); break;
            default:
                printf("Option de filtre (24-bits) invalide. Réessayez.\n");
        }
    }
}