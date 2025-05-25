#ifndef BMP8_H
#define BMP8_H

typedef struct {
    unsigned char header[54];
    unsigned char colorTable[1024];
    unsigned char *data;
    unsigned int width;
    unsigned int height;
    unsigned int colorDepth;
    unsigned int dataSize;
} t_bmp8;

t_bmp8 *bmp8_loadImage(const char *filename);
void bmp8_saveImage(const char *filename, t_bmp8 *img);
void bmp8_free(t_bmp8 *img);
void bmp8_printInfo(t_bmp8 *img);
void bmp8_negative(t_bmp8 *img);
void bmp8_brightness(t_bmp8 *img, int value);
void bmp8_threshold(t_bmp8 *img, int threshold);

// Filtres
void bmp8_applyFilter(t_bmp8 *img, float kernel[3][3], float factor, int bias);
void bmp8_boxBlur(t_bmp8 *img);
void bmp8_gaussianBlur(t_bmp8 *img);
void bmp8_outline(t_bmp8 *img);
void bmp8_emboss(t_bmp8 *img);
void bmp8_sharpen(t_bmp8 *img);

// Histogramme
unsigned int *bmp8_computeHistogram(t_bmp8 *img);
unsigned int *bmp8_computeCDF(unsigned int *hist);
void bmp8_equalize(t_bmp8 *img, unsigned int *hist_eq);
void bmp8_equalizeHistogram(t_bmp8 *img);
void bmp8_printHistogram(t_bmp8 *img);
#endif
