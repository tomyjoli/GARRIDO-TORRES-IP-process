#ifndef BMP24_H
#define BMP24_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} t_bmp_header;
#pragma pack(pop)

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} t_pixel24;

typedef struct {
    t_bmp_header header;
    t_pixel24 *data;
} t_bmp24;

// Fonctions de base
t_bmp24 *bmp24_loadImage(const char *filename);
void bmp24_saveImage(const char *filename, t_bmp24 *img);
void bmp24_free(t_bmp24 *img);
void bmp24_printInfo(t_bmp24 *img);

// Fonctions de traitement
void bmp24_negative(t_bmp24 *img);
void bmp24_brightness(t_bmp24 *img, int value);
void bmp24_threshold(t_bmp24 *img, int threshold);
void bmp24_grayscale(t_bmp24 *img);
void bmp24_sepia(t_bmp24 *img);

// Filtres de convolution
void bmp24_boxBlur(t_bmp24 *img);
void bmp24_gaussianBlur(t_bmp24 *img);
void bmp24_outline(t_bmp24 *img);
void bmp24_emboss(t_bmp24 *img);
void bmp24_sharpen(t_bmp24 *img);

// Fonctions d'histogramme
void bmp24_computeHistogram(t_bmp24 *img, unsigned int hist_r[256], unsigned int hist_g[256], unsigned int hist_b[256]);
void bmp24_equalizeHistogram(t_bmp24 *img);
#endif //BMP24_H
