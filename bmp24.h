#ifndef BMP24_H_
#define BMP24_H_

#include <stdint.h> // Pour les types entiers (uint8_t, uint16_t, uint32_t, int32_t)
#include <stdio.h>  // Pour FILE

// ----- Constantes (PDF Section 2.2.2 et autres) -----
#define BMP_TYPE_SIGNATURE    0x4D42 // 'BM' en little-endian
#define DEFAULT_COLOR_DEPTH_24 24
#define FILE_HEADER_SIZE      14   // Taille de t_bmp_header
#define INFO_HEADER_SIZE      40   // Taille de t_bmp_info (BITMAPINFOHEADER)

// ----- Structures (PDF Section 2.2) -----

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

// t_bmp_header : File Header (14 octets)
// Doit être packé pour correspondre exactement à la structure du fichier.
struct s_bmp_header {
    uint16_t type;              // Signature 'BM' (0x4D42)
    uint32_t size;              // Taille totale du fichier en octets
    uint16_t reserved1;         // Réservé, doit être 0
    uint16_t reserved2;         // Réservé, doit être 0
    uint32_t offset;            // Offset des données pixel depuis le début du fichier
}__attribute__((packed));
typedef struct s_bmp_header t_bmp_header;


// t_bmp_info : Info Header (BITMAPINFOHEADER, 40 octets)
// Doit être packé.
struct s_bmp_info{
    uint32_t size;              // Taille de cette structure (40 octets)
    int32_t  width;             // Largeur de l'image en pixels
    int32_t  height;            // Hauteur de l'image en pixels (peut être négative)
    uint16_t planes;            // Nombre de plans de couleur (doit être 1)
    uint16_t bits_per_pixel;    // Bits par pixel (ex: 24 pour 24bpp)
    uint32_t compression;       // Type de compression (0 pour BI_RGB, non compressé)
    uint32_t image_size;        // Taille des données pixel brutes (avec padding), peut être 0 si non compressé
    int32_t  x_pixels_per_meter;// Résolution horizontale (pixels/mètre)
    int32_t  y_pixels_per_meter;// Résolution verticale (pixels/mètre)
    uint32_t ncolors;           // Nombre de couleurs dans la palette (0 pour 24bpp)
    uint32_t importantcolors;   // Nombre de couleurs importantes (0 = toutes)
}__attribute__((packed));
typedef struct s_bmp_info t_bmp_info;

// Structure principale pour l'image BMP 24 bits
typedef struct {
    t_bmp_header header;        // Header du fichier BMP
    t_bmp_info   info_header;   // Header d'information DIB

    // Champs de commodité dérivés des headers
    int width;                  // Largeur de l'image (toujours positive)
    int height;                 // Hauteur de l'image (conserve le signe original du header)
    // Dans img->data, la hauteur est traitée comme positive (abs(height))
    // et les données sont stockées top-down.
    int colorDepth;             // Profondeur de couleur (devrait être 24)

    t_pixel **data;             // Données pixel (tableau 2D [hauteur_abs][largeur])
    // Stockées en interne top-down (ligne 0 = ligne du haut)
} t_bmp24;


// ----- Fonctions d'Aide pour la Lecture/Écriture Brute (non partie de l'API publique du PDF mais utiles) -----
void file_rawRead (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file);
void file_rawWrite (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file);

// ----- Fonctions d'Allocation et de Libération (PDF Section 2.3) -----
t_pixel **bmp24_allocateDataPixels(int width, int height_abs);
void bmp24_freeDataPixels(t_pixel **pixels, int height_abs);
t_bmp24 *bmp24_allocate(int width, int height, int colorDepth); // `height` peut être négatif ici
void bmp24_free(t_bmp24 *img);

// ----- Lecture et Écriture d'Image (PDF Section 2.4) -----
t_bmp24 *bmp24_loadImage(const char *filename);
void bmp24_saveImage(const char *filename, t_bmp24 *img);
void bmp24_printInfo(t_bmp24 *img);

// ----- Traitement d'Image (PDF Section 2.5) -----
uint8_t clamp_pixel_value(int value); // Fonction utilitaire
void bmp24_negative(t_bmp24 *img);
void bmp24_grayscale(t_bmp24 *img);
void bmp24_brightness(t_bmp24 *img, int value);
void bmp24_threshold(t_bmp24 *img, int threshold_val);

// ----- Filtres de Convolution (PDF Section 2.6) -----
void bmp24_apply_filter_generic(t_bmp24 *img, float kernel[3][3], float factor, int bias);
void bmp24_boxBlur(t_bmp24 *img);
void bmp24_gaussianBlur(t_bmp24 *img);
void bmp24_outline(t_bmp24 *img);
void bmp24_emboss(t_bmp24 *img);
void bmp24_sharpen(t_bmp24 *img);

// ----- Égalisation d'Histogramme Couleur (PDF Section 3) -----
void bmp24_equalize(t_bmp24 *img);

#endif // BMP24_H_