#ifndef BMP24_H_
#define BMP24_H_

#include <stdint.h>
#include <stdio.h>

// Constantes
#define BMP_TYPE_SIGNATURE    0x4D42
#define DEFAULT_COLOR_DEPTH_24 24
#define FILE_HEADER_SIZE      14
#define INFO_HEADER_SIZE      40

//Structures

// Structure pour un pixel couleur (stocké en RGB en mémoire)
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} t_pixel;

// Structure pour un pixel YUV (utilisé pour l'égalisation)
typedef struct {
    float y;
    float u;
    float v;
} t_yuv;

struct s_bmp_header {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
}__attribute__((packed));
typedef struct s_bmp_header t_bmp_header;


// t_bmp_info : Info Header
struct s_bmp_info{
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_pixels_per_meter;
    int32_t  y_pixels_per_meter;
    uint32_t ncolors;
    uint32_t importantcolors;
}__attribute__((packed));
typedef struct s_bmp_info t_bmp_info;

// Structure principale pour l'image BMP 24 bits
typedef struct {
    t_bmp_header header;
    t_bmp_info   info_header;

    int width;
    int height;
    // Dans img->data, la hauteur est traitée comme positive (abs(height))
    int colorDepth;

    t_pixel **data;
} t_bmp24;


// Fonctions d'Aide pour la Lecture/Écriture Brute
void file_rawRead (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file);
void file_rawWrite (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file);

// Fonctions d'Allocation et de Libération
t_pixel **bmp24_allocateDataPixels(int width, int height_abs);
void bmp24_freeDataPixels(t_pixel **pixels, int height_abs);
t_bmp24 *bmp24_allocate(int width, int height, int colorDepth);
void bmp24_free(t_bmp24 *img);

// Lecture et Écriture d'Image
t_bmp24 *bmp24_loadImage(const char *filename);
void bmp24_saveImage(const char *filename, t_bmp24 *img);
void bmp24_printInfo(t_bmp24 *img);

// Traitement d'Image
uint8_t clamp_pixel_value(int value);
void bmp24_negative(t_bmp24 *img);
void bmp24_grayscale(t_bmp24 *img);
void bmp24_brightness(t_bmp24 *img, int value);
void bmp24_threshold(t_bmp24 *img, int threshold_val);

// Filtres de Convolution
void bmp24_apply_filter_generic(t_bmp24 *img, float kernel[3][3], float factor, int bias);
void bmp24_boxBlur(t_bmp24 *img);
void bmp24_gaussianBlur(t_bmp24 *img);
void bmp24_outline(t_bmp24 *img);
void bmp24_emboss(t_bmp24 *img);
void bmp24_sharpen(t_bmp24 *img);

// Égalisation d'Histogramme Couleur
void bmp24_equalize(t_bmp24 *img);

#endif