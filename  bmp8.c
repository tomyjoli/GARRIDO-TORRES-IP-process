#include <stdio.h>
#include <stdlib.h>
#include "bmp8.h"

t_bmp8 *bmp8_loadImage(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erreur ouverture fichier");
        return NULL;
    }

    t_bmp8 *img = (t_bmp8 *)malloc(sizeof(t_bmp8));
    if (!img) {
        perror("Erreur malloc image");
        fclose(file);
        return NULL;
    }

    fread(img->header, sizeof(unsigned char), 54, file);

    img->width = *(unsigned int *)&img->header[18];
    img->height = *(unsigned int *)&img->header[22];
    img->colorDepth = *(unsigned short *)&img->header[28];
    img->dataSize = *(unsigned int *)&img->header[34];

    if (img->colorDepth != 8) {
        fprintf(stderr, "Erreur : image n'est pas en 8 bits.\n");
        free(img);
        fclose(file);
        return NULL;
    }

    fread(img->colorTable, sizeof(unsigned char), 1024, file);

    img->data = (unsigned char *)malloc(img->dataSize);
    if (!img->data) {
        perror("Erreur malloc data");
        free(img);
        fclose(file);
        return NULL;
    }

    fread(img->data, sizeof(unsigned char), img->dataSize, file);

    fclose(file);
    return img;
}

void bmp8_saveImage(const char *filename, t_bmp8 *img) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Erreur ouverture fichier pour écriture");
        return;
    }

    fwrite(img->header, sizeof(unsigned char), 54, file);
    fwrite(img->colorTable, sizeof(unsigned char), 1024, file);
    fwrite(img->data, sizeof(unsigned char), img->dataSize, file);

    fclose(file);
}

void bmp8_free(t_bmp8 *img) {
    if (img) {
        free(img->data);
        free(img);
    }
}

void bmp8_printInfo(t_bmp8 *img) {
    if (img) {
        printf("Infos de l'image :\n");
        printf("Largeur : %u pixels\n", img->width);
        printf("Hauteur : %u pixels\n", img->height);
        printf("Profondeur de couleur : %u bits\n", img->colorDepth);
        printf("Taille des données : %u octets\n", img->dataSize);
    }
}

void bmp8_negative(t_bmp8 *img) {
    for (unsigned int i = 0; i < img->dataSize; i++) {
        img->data[i] = 255 - img->data[i];
    }
}

void bmp8_brightness(t_bmp8 *img, int value) {
    for (unsigned int i = 0; i < img->dataSize; i++) {
        int pixel = img->data[i] + value;
        if (pixel > 255) pixel = 255;
        if (pixel < 0) pixel = 0;
        img->data[i] = (unsigned char)pixel;
    }
}

void bmp8_threshold(t_bmp8 *img, int threshold) {
    for (unsigned int i = 0; i < img->dataSize; i++) {
        img->data[i] = (img->data[i] >= threshold) ? 255 : 0;
    }
}

// Fonction pour appliquer un filtre générique
void bmp8_applyFilter(t_bmp8 *img, float kernel[3][3], float factor, int bias) {
    unsigned char *newData = (unsigned char *)malloc(img->dataSize);
    if (!newData) return;

    for (unsigned int y = 1; y < img->height - 1; y++) {
        for (unsigned int x = 1; x < img->width - 1; x++) {
            float sum = 0.0;

            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    unsigned int pos = (y + ky) * img->width + (x + kx);
                    sum += img->data[pos] * kernel[ky + 1][kx + 1];
                }
            }

            sum = sum * factor + bias;
            if (sum > 255) sum = 255;
            if (sum < 0) sum = 0;

            newData[y * img->width + x] = (unsigned char)sum;
        }
    }

    // Copier les bords non modifiés
    for (unsigned int i = 0; i < img->width; i++) {
        newData[i] = img->data[i]; // Haut
        newData[(img->height - 1) * img->width + i] = img->data[(img->height - 1) * img->width + i]; // Bas
    }
    for (unsigned int i = 0; i < img->height; i++) {
        newData[i * img->width] = img->data[i * img->width]; // Gauche
        newData[i * img->width + img->width - 1] = img->data[i * img->width + img->width - 1]; // Droite
    }

    free(img->data);
    img->data = newData;
}

// Filtres prédéfinis
void bmp8_boxBlur(t_bmp8 *img) {
    float kernel[3][3] = {
        {1.0/9.0, 1.0/9.0, 1.0/9.0},
        {1.0/9.0, 1.0/9.0, 1.0/9.0},
        {1.0/9.0, 1.0/9.0, 1.0/9.0}
    };
    bmp8_applyFilter(img, kernel, 1.0, 0);
}

void bmp8_gaussianBlur(t_bmp8 *img) {
    float kernel[3][3] = {
        {1.0/16.0, 2.0/16.0, 1.0/16.0},
        {2.0/16.0, 4.0/16.0, 2.0/16.0},
        {1.0/16.0, 2.0/16.0, 1.0/16.0}
    };
    bmp8_applyFilter(img, kernel, 1.0, 0);
}

void bmp8_outline(t_bmp8 *img) {
    float kernel[3][3] = {
        {-1, -1, -1},
        {-1,  8, -1},
        {-1, -1, -1}
    };
    bmp8_applyFilter(img, kernel, 1.0, 0);
}

void bmp8_emboss(t_bmp8 *img) {
    float kernel[3][3] = {
        {-2, -1,  0},
        {-1,  1,  1},
        { 0,  1,  2}
    };
    bmp8_applyFilter(img, kernel, 1.0, 128);
}

void bmp8_sharpen(t_bmp8 *img) {
    float kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };
    bmp8_applyFilter(img, kernel, 1.0, 0);
}

// Fonction pour calculer l'histogramme
unsigned int *bmp8_computeHistogram(t_bmp8 *img) {
    unsigned int *histogram = (unsigned int *)malloc(256 * sizeof(unsigned int));
    if (!histogram) return NULL;

    for (int i = 0; i < 256; i++) histogram[i] = 0;

    for (unsigned int i = 0; i < img->dataSize; i++) {
        histogram[img->data[i]]++;
    }

    return histogram;
}

unsigned int *bmp8_computeCDF(unsigned int *hist) {
    unsigned int *cdf = (unsigned int *)malloc(256 * sizeof(unsigned int));
    if (!cdf) return NULL;

    cdf[0] = hist[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i-1] + hist[i];
    }

    return cdf;
}

void bmp8_equalize(t_bmp8 *img, unsigned int *hist_eq) {
    unsigned char lut[256];
    float scale = 255.0f / (img->width * img->height);

    for (int i = 0; i < 256; i++) {
        lut[i] = (unsigned char)round(hist_eq[i] * scale);
    }

    for (unsigned int i = 0; i < img->dataSize; i++) {
        img->data[i] = lut[img->data[i]];
    }
}

void bmp8_equalizeHistogram(t_bmp8 *img) {
    unsigned int *histogram = bmp8_computeHistogram(img);
    if (!histogram) return;

    unsigned int *cdf = bmp8_computeCDF(histogram);
    if (!cdf) {
        free(histogram);
        return;
    }

    bmp8_equalize(img, cdf);

    free(histogram);
    free(cdf);
}

void bmp8_printHistogram(t_bmp8 *img) {
    unsigned int *histogram = bmp8_computeHistogram(img);
    if (!histogram) return;

    printf("\nHistogramme :\n");
    for (int i = 0; i < 256; i++) {
        printf("%3d: %5d | ", i, histogram[i]);

        // Affichage graphique simplifié
        int bars = histogram[i] / (img->width * img->height / 100);
        for (int j = 0; j < bars; j++) {
            printf("#");
        }
        printf("\n");
    }

    free(histogram);
}