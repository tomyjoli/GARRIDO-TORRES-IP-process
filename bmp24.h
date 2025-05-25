#ifndef BMP24_H
#define BMP24_H

#include <stdint.h> // Pour les types entiers standardisés comme uint16_t, uint32_t, int32_t, uint8_t
#include <stdio.h>  // Pour le type FILE (utilisé dans les prototypes)
#include <stdlib.h> // Pour size_t (implicitement via malloc/free dans les prototypes) et abs()
#include <math.h>   // Pour roundf (utilisé dans l'implémentation des filtres)

// ----- Constantes utiles-----
#define BMP_TYPE_SIGNATURE     0x4D42 // 'BM'
#define FILE_HEADER_SIZE       14
#define INFO_HEADER_SIZE       40
#define DEFAULT_COLOR_DEPTH_24 24

// ----- Définition des Structures -----

// t_pixel 
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} t_pixel;

// t_bmp_header 
#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} t_bmp_header;
#pragma pack(pop)

// t_bmp_info 
#pragma pack(push, 1)
typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bits_per_pixel; // Nommé 'bits' dans la structure du PDF
    uint32_t compression;
    uint32_t image_size;     // Nommé 'imagesize' dans la structure du PDF
    int32_t  x_pixels_per_meter; // Nommé 'xresolution'
    int32_t  y_pixels_per_meter; // Nommé 'yresolution'
    uint32_t ncolors;
    uint32_t importantcolors;
} t_bmp_info;
#pragma pack(pop)

// t_bmp24 
typedef struct {
    t_bmp_header header;
    t_bmp_info   info_header;
    int width;                  // Redondant avec info_header.width
    int height;                 // Redondant avec info_header.height
    int colorDepth;             // Redondant avec info_header.bits_per_pixel
    t_pixel **data;
} t_bmp24;

// Suggestion pour stocker les composantes YUV (flottantes)
typedef struct {
    float y;
    float u;
    float v;
} t_yuv;

// ----- Prototypes des Fonctions -----

// Fonctions d'aide pour la lecture/écriture bas niveau question 2.4.1
void file_rawRead (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file);
void file_rawWrite (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file);

// Fonctions d'Allocation et de Libération question 2.3
t_pixel **bmp24_allocateDataPixels(int width, int height_abs);
void bmp24_freeDataPixels(t_pixel **pixels, int height_abs);
t_bmp24 *bmp24_allocate(int width, int height, int colorDepth);
void bmp24_free(t_bmp24 *img);

// Fonctions de Lecture et Écriture d'Image question 2.4
t_bmp24 *bmp24_loadImage(const char *filename); // PDF question 2.4.3
void bmp24_saveImage(const char *filename, t_bmp24 *img); // PDF question 2.4.4

// Fonction utilitaire pour afficher les informations
void bmp24_printInfo(t_bmp24 *img);

// Fonctions de Traitement d'Image PDF question 2.5
void bmp24_negative(t_bmp24 *img);
void bmp24_grayscale(t_bmp24 *img);
void bmp24_brightness(t_bmp24 *img, int value);
void bmp24_threshold(t_bmp24 *img, int threshold_val);

// Fonctions pour les Filtres de Convolution PDF question 2.6
// Pour simplifier, on fait une fonction qui applique à toute l'image.
void bmp24_apply_filter_generic(t_bmp24 *img, float kernel[3][3], float factor, int bias);

void bmp24_boxBlur(t_bmp24 *img);
void bmp24_gaussianBlur(t_bmp24 *img);
void bmp24_outline(t_bmp24 *img);
void bmp24_emboss(t_bmp24 *img);
void bmp24_sharpen(t_bmp24 *img);

// Fonction principale pour l'égalisation d'histogramme couleur question 3.4.3
void bmp24_equalize(t_bmp24 *img);

#endif //BMP24_H