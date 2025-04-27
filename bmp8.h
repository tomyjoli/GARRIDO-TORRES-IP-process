



#ifndef BMP8_H // Gardien d'inclusion - Début
#define BMP8_H


// Définition de la structure
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


void bmp8_freeImage(t_bmp8 *image);

#endif // BMP8_H - Gardien d'inclusion - Fin

#ifndef GARRIDO_TORRES_IP_PROCESS_BMP8_H
#define GARRIDO_TORRES_IP_PROCESS_BMP8_H

#endif //GARRIDO_TORRES_IP_PROCESS_BMP8_H
