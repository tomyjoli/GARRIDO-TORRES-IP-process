#include <stdio.h>
#include <stdlib.h>
#include "bmp8.h"

// En supposant que le fichier image BMP est ouvert et
// que l'en-tête est stocké dans le tableau header
//unsigned char header[54];
// On sait que la largeur de l'image est stockée à l'offset 18 de l'en-tête
//unsigned int width = *(unsigned int *)&header[18];


t_bmp8 * bmp8_loadImage(const char * filename){
    FILE*file=fopen(filename,"rb");
    if (!file){
        perror("impossible d'ouvrir le fichier");
        return NULL;
    }
    unsigned char header[54];
    if (fread(header, sizeof(unsigned char), 54, file) != 54) {
        perror("Erreur lors de la lecture de l'en-tête");
        fclose(file);
        return NULL;
    }
    unsigned short colorDepth = *(unsigned short *)&header[28];
    if (colorDepth != 8) {
        fprintf(stderr, "L'image n'a pas une profondeur de 8 bits\n");
        fclose(file);
        return NULL;
    }

    // Extraire les informations de l'en-tête
    unsigned int width = *(unsigned int *)&header[18];
    unsigned int height = *(unsigned int *)&header[22];
    unsigned int dataOffset = *(unsigned int *)&header[10];
    unsigned int dataSize = *(unsigned int *)&header[34];

    // Allouer de la mémoire pour la structure de l'image
    t_bmp8 *img = (t_bmp8 *)malloc(sizeof(t_bmp8));
    if (!img) {
        perror("Impossible d'allouer de la mémoire pour l'image");
        fclose(file);
        return NULL;
    }

    // Copier les champs de l'en-tête dans la structure
    for (int i = 0; i < 54; i++) {
        img->header[i] = header[i];
    }

    img->width = width;
    img->height = height;
    img->colorDepth = colorDepth;
    img->dataSize = dataSize;

    // Allouer de la mémoire pour les données de l'image
    img->data = (unsigned char *)malloc(dataSize);
    if (!img->data) {
        perror("Impossible d'allouer de la mémoire pour les données de l'image");
        free(img);
        fclose(file);
        return NULL;
    }

    // Lire les données de l'image
    fseek(file, dataOffset, SEEK_SET);
    if (fread(img->data, sizeof(unsigned char), dataSize, file) != dataSize) {
        perror("Erreur lors de la lecture des données de l'image");
        free(img->data);
        free(img);
        fclose(file);
        return NULL;
    }

    // Fermer le fichier
    fclose(file);
    return img;

    }



//void bmp8_saveImage(const char * filename, t_bmp8 * img);
//void bmp8_free(t_bmp8 * img);
//void bmp8_printInfo(t_bmp8 * img);