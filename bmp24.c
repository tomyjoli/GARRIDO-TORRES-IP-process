#include "bmp24.h"
#include <math.h>

t_bmp24 *bmp24_loadImage(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;

    t_bmp24 *img = malloc(sizeof(t_bmp24));
    if (!img) {
        fclose(file);
        return NULL;
    }

    // Lire l'en-tête
    if (fread(&img->header, sizeof(t_bmp_header), 1, file) != 1) {
        free(img);
        fclose(file);
        return NULL;
    }

    // Vérifier que c'est bien une image 24 bits
    if (img->header.bits_per_pixel != 24) {
        free(img);
        fclose(file);
        return NULL;
    }

    // Allouer la mémoire pour les données
    img->data = malloc(img->header.image_size);
    if (!img->data) {
        free(img);
        fclose(file);
        return NULL;
    }

    // Lire les données
    fseek(file, img->header.offset, SEEK_SET);
    if (fread(img->data, img->header.image_size, 1, file) != 1) {
        free(img->data);
        free(img);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return img;
}

void bmp24_saveImage(const char *filename, t_bmp24 *img) {
    FILE *file = fopen(filename, "wb");
    if (!file) return;

    fwrite(&img->header, sizeof(t_bmp_header), 1, file);
    fseek(file, img->header.offset, SEEK_SET);
    fwrite(img->data, img->header.image_size, 1, file);

    fclose(file);
}

void bmp24_free(t_bmp24 *img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

void bmp24_printInfo(t_bmp24 *img) {
    printf("Information sur l'image 24 bits:\n");
    printf("Largeur: %d\n", img->header.width);
    printf("Hauteur: %d\n", img->header.height);
    printf("Taille de l'image: %d bytes\n", img->header.image_size);
    printf("Bits par pixel: %d\n", img->header.bits_per_pixel);
}

void bmp24_negative(t_bmp24 *img) {
    for (uint32_t i = 0; i < img->header.image_size / 3; i++) {
        img->data[i].r = 255 - img->data[i].r;
        img->data[i].g = 255 - img->data[i].g;
        img->data[i].b = 255 - img->data[i].b;
    }
}

void bmp24_brightness(t_bmp24 *img, int value) {
    for (uint32_t i = 0; i < img->header.image_size / 3; i++) {
        int r = img->data[i].r + value;
        int g = img->data[i].g + value;
        int b = img->data[i].b + value;

        img->data[i].r = (r < 0) ? 0 : (r > 255) ? 255 : r;
        img->data[i].g = (g < 0) ? 0 : (g > 255) ? 255 : g;
        img->data[i].b = (b < 0) ? 0 : (b > 255) ? 255 : b;
    }
}

void bmp24_threshold(t_bmp24 *img, int threshold) {
    for (uint32_t i = 0; i < img->header.image_size / 3; i++) {
        uint8_t gray = (img->data[i].r + img->data[i].g + img->data[i].b) / 3;
        if (gray >= threshold) {
            img->data[i].r = img->data[i].g = img->data[i].b = 255;
        } else {
            img->data[i].r = img->data[i].g = img->data[i].b = 0;
        }
    }
}

void bmp24_grayscale(t_bmp24 *img) {
    for (uint32_t i = 0; i < img->header.image_size / 3; i++) {
        uint8_t gray = (img->data[i].r + img->data[i].g + img->data[i].b) / 3;
        img->data[i].r = img->data[i].g = img->data[i].b = gray;
    }
}

void bmp24_sepia(t_bmp24 *img) {
    for (uint32_t i = 0; i < img->header.image_size / 3; i++) {
        uint8_t r = img->data[i].r;
        uint8_t g = img->data[i].g;
        uint8_t b = img->data[i].b;

        img->data[i].r = (r * 0.393 + g * 0.769 + b * 0.189) > 255 ? 255 : (r * 0.393 + g * 0.769 + b * 0.189);
        img->data[i].g = (r * 0.349 + g * 0.686 + b * 0.168) > 255 ? 255 : (r * 0.349 + g * 0.686 + b * 0.168);
        img->data[i].b = (r * 0.272 + g * 0.534 + b * 0.131) > 255 ? 255 : (r * 0.272 + g * 0.534 + b * 0.131);
    }
}
