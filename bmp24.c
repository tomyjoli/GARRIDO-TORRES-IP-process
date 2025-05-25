#include "bmp24.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//Fonctions d'Aide pour la Lecture/Écriture
void file_rawRead (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file) {
    if (!file || !buffer) {
        fprintf(stderr, "file_rawRead: Erreur - Pointeur de fichier ou buffer NULL.\n");
        return;
    }
    if (fseek(file, (long)position, SEEK_SET) != 0) {
        perror("file_rawRead: Erreur fseek");
        return;
    }
    if (fread(buffer, size_element, n_elements, file) != n_elements) {
        if (feof(file)) {
        }
        else if (ferror(file)) {
            perror("file_rawRead: Erreur fread");
        }
        // L'appelant doit vérifier ferror(file) ou les données lues si nécessaire.
    }
}

void file_rawWrite (uint32_t position, void * buffer, uint32_t size_element, size_t n_elements, FILE * file) {
    if (!file || !buffer) {
        fprintf(stderr, "file_rawWrite: Erreur - Pointeur de fichier ou buffer NULL.\n");
        return;
    }
    if (fseek(file, (long)position, SEEK_SET) != 0) {
        perror("file_rawWrite: Erreur fseek");
        return;
    }
    if (fwrite(buffer, size_element, n_elements, file) != n_elements) {
        perror("file_rawWrite: Erreur fwrite");
    }
}

// Fonctions d'Allocation et de Libération
t_pixel **bmp24_allocateDataPixels(int width, int height_abs) {
    if (width <= 0 || height_abs <= 0) {
        fprintf(stderr, "bmp24_allocateDataPixels: Dimensions invalides (W:%d x H_abs:%d).\n", width, height_abs);
        return NULL;
    }
    t_pixel **pixels = (t_pixel **)malloc((size_t)height_abs * sizeof(t_pixel *));
    if (!pixels) {
        perror("bmp24_allocateDataPixels: Erreur malloc pour les pointeurs de lignes");
        return NULL;
    }
    for (int i = 0; i < height_abs; ++i) {
        pixels[i] = (t_pixel *)calloc((size_t)width, sizeof(t_pixel));
        if (!pixels[i]) {
            perror("bmp24_allocateDataPixels: Erreur calloc pour une ligne de pixels");
            for (int j = 0; j < i; ++j) {
                free(pixels[j]);
            }
            free(pixels);
            return NULL;
        }
    }
    return pixels;
}

void bmp24_freeDataPixels(t_pixel **pixels, int height_abs) {
    if (!pixels || height_abs <= 0) {
        return;
    }
    for (int i = 0; i < height_abs; ++i) {
        if (pixels[i]) {
            free(pixels[i]);
            pixels[i] = NULL;
        }
    }
    free(pixels);
}

t_bmp24 *bmp24_allocate(int width, int height_signed, int colorDepth) {
    if (width <= 0 || height_signed == 0) {
        fprintf(stderr, "bmp24_allocate: Dimensions invalides (W:%d x H:%d).\n", width, height_signed);
        return NULL;
    }
    if (colorDepth != DEFAULT_COLOR_DEPTH_24) {
        fprintf(stderr, "bmp24_allocate: Profondeur de couleur non supportée (%d), attendu %d.\n", colorDepth, DEFAULT_COLOR_DEPTH_24);
        return NULL;
    }

    t_bmp24 *img = (t_bmp24 *)malloc(sizeof(t_bmp24));
    if (!img) {
        perror("bmp24_allocate: Erreur malloc pour t_bmp24");
        return NULL;
    }
    memset(img, 0, sizeof(t_bmp24));

    int height_abs = abs(height_signed);
    img->data = bmp24_allocateDataPixels(width, height_abs);
    if (!img->data) {
        free(img);
        return NULL;
    }

    img->width = width;
    img->height = height_signed;
    img->colorDepth = colorDepth;

    // Initialiser les headers pour une NOUVELLE image (sera écrasé lors du chargement)
    img->header.type = BMP_TYPE_SIGNATURE;
    img->header.reserved1 = 0;
    img->header.reserved2 = 0;
    img->header.offset = (uint32_t)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

    img->info_header.size = INFO_HEADER_SIZE;
    img->info_header.width = width;
    img->info_header.height = height_signed;
    img->info_header.planes = 1;
    img->info_header.bits_per_pixel = (uint16_t)colorDepth;
    img->info_header.compression = 0;

    uint32_t row_stride_bytes = ((uint32_t)width * (img->info_header.bits_per_pixel / 8) + 3) & ~3u;
    img->info_header.image_size = row_stride_bytes * (uint32_t)height_abs;

    img->info_header.x_pixels_per_meter = 2835;
    img->info_header.y_pixels_per_meter = 2835;
    img->info_header.ncolors = 0;
    img->info_header.importantcolors = 0;

    img->header.size = img->header.offset + img->info_header.image_size;

    return img;
}

void bmp24_free(t_bmp24 *img) {
    if (img) {
        if (img->data) {
            bmp24_freeDataPixels(img->data, abs(img->height));
        }
        free(img);
    }
}

// Lecture et Écriture d'Image
t_bmp24 *bmp24_loadImage(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("bmp24_loadImage: Erreur ouverture fichier");
        return NULL;
    }

    t_bmp_header file_h_read;
    t_bmp_info info_h_read;

    // 1. Lire t_bmp_header
    file_rawRead(0, &file_h_read, sizeof(t_bmp_header), 1, file);
    if (ferror(file) || (feof(file) && sizeof(t_bmp_header) > 0)) {
        fprintf(stderr, "bmp24_loadImage: Erreur ou EOF pendant lecture t_bmp_header.\n");
        if (ferror(file)) perror("bmp24_loadImage (t_bmp_header)");
        fclose(file); return NULL;
    }

    // 2. Valider t_bmp_header
    if (file_h_read.type != BMP_TYPE_SIGNATURE) {
        fprintf(stderr, "bmp24_loadImage: Signature BMP invalide (lu: 0x%X, attendu: 0x%X).\n", file_h_read.type, BMP_TYPE_SIGNATURE);
        fclose(file); return NULL;
    }

    // 3. Lire t_bmp_info
    file_rawRead(FILE_HEADER_SIZE, &info_h_read, sizeof(t_bmp_info), 1, file);
    if (ferror(file) || (feof(file) && sizeof(t_bmp_info) > 0)) {
        fprintf(stderr, "bmp24_loadImage: Erreur ou EOF pendant lecture t_bmp_info.\n");
        if (ferror(file)) perror("bmp24_loadImage (t_bmp_info)");
        fclose(file); return NULL;
    }

    // 4. Valider t_bmp_info
    if (info_h_read.size < INFO_HEADER_SIZE) {
        fprintf(stderr, "bmp24_loadImage: Taille DIB header (%u) incorrecte, attendu au moins %d.\n", info_h_read.size, INFO_HEADER_SIZE);
        fclose(file); return NULL;
    }
    if (info_h_read.bits_per_pixel != DEFAULT_COLOR_DEPTH_24) {
        fprintf(stderr, "bmp24_loadImage: Image non 24-bits (bits_per_pixel: %u).\n", info_h_read.bits_per_pixel);
        fclose(file); return NULL;
    }
    if (info_h_read.compression != 0) { // 0 pour BI_RGB (non compressé)
        fprintf(stderr, "bmp24_loadImage: Compression non supportée (type: %u).\n", info_h_read.compression);
        fclose(file); return NULL;
    }
    if (info_h_read.width <= 0 || info_h_read.height == 0) {
        fprintf(stderr, "bmp24_loadImage: Dimensions d'image invalides dans header (W:%d, H:%d).\n", info_h_read.width, info_h_read.height);
        fclose(file); return NULL;
    }

    // 5. Allouer la structure t_bmp24
    t_bmp24 *img = bmp24_allocate(info_h_read.width, info_h_read.height, info_h_read.bits_per_pixel);
    if (!img) {
        fclose(file); return NULL;
    }

    // 6. Copier les headers lus dans la structure img
    img->header = file_h_read;
    img->info_header = info_h_read;

    // 7. Se positionner pour lire les données pixel
    if (fseek(file, (long)img->header.offset, SEEK_SET) != 0) {
        perror("bmp24_loadImage: Erreur fseek vers données pixel");
        bmp24_free(img); fclose(file); return NULL;
    }

    // 8. Lire les données pixel
    uint32_t bytes_per_pixel = img->info_header.bits_per_pixel / 8;
    uint32_t row_padded_size = ((uint32_t)img->width * bytes_per_pixel + 3) & ~3u;

    int height_abs_val = abs(img->height); // Utiliser la valeur absolue pour les calculs de taille
    uint32_t calculated_image_size = row_padded_size * (uint32_t)height_abs_val;
    if (img->info_header.image_size == 0) {
        img->info_header.image_size = calculated_image_size;
    } else if (img->info_header.image_size != calculated_image_size) {
        fprintf(stderr, "bmp24_loadImage: Avertissement - image_size du header (%u) ne correspond pas à la taille calculée (%u).\n",
                img->info_header.image_size, calculated_image_size);

    }


    uint8_t *row_buffer = (uint8_t *)malloc(row_padded_size);
    if (!row_buffer) {
        perror("bmp24_loadImage: Erreur malloc row_buffer");
        bmp24_free(img); fclose(file); return NULL;
    }

    for (int i = 0; i < height_abs_val; ++i) {
        size_t bytes_read = fread(row_buffer, 1, row_padded_size, file);
        if (bytes_read != row_padded_size) {
            fprintf(stderr, "bmp24_loadImage: Erreur lecture ligne pixel %d. Attendu %u, lu %zu.\n", i, row_padded_size, bytes_read);
            if (feof(file)) fprintf(stderr, " (Fin de fichier atteinte prématurément)\n");
            else if(ferror(file)) perror(" (Erreur fread)");
            free(row_buffer); bmp24_free(img); fclose(file); return NULL;
        }


        int storage_y = (img->info_header.height > 0) ? (height_abs_val - 1 - i) : i;

        for (int x = 0; x < img->width; ++x) {
            // Les pixels BMP sont stockés en BGR
            img->data[storage_y][x].blue  = row_buffer[x * bytes_per_pixel + 0];
            img->data[storage_y][x].green = row_buffer[x * bytes_per_pixel + 1];
            img->data[storage_y][x].red   = row_buffer[x * bytes_per_pixel + 2];
        }
    }
    free(row_buffer);
    fclose(file);
    printf("Image '%s' chargée avec succès (%dx%d, %dbpp).\n", filename, img->width, img->height, img->colorDepth);
    return img;
}

void bmp24_saveImage(const char *filename, t_bmp24 *img) {
    if (!img || !img->data) {
        fprintf(stderr, "bmp24_saveImage: Image ou données invalides.\n");
        return;
    }

    // Vérification critique du packing des structures
    if (sizeof(t_bmp_header) != FILE_HEADER_SIZE) {
        fprintf(stderr, "ERREUR CRITIQUE SAVE: sizeof(t_bmp_header) est %zu, attendu %d! Problème de packing?\n", sizeof(t_bmp_header), FILE_HEADER_SIZE);
        return;
    }
    if (sizeof(t_bmp_info) != INFO_HEADER_SIZE) {
        fprintf(stderr, "ERREUR CRITIQUE SAVE: sizeof(t_bmp_info) est %zu, attendu %d! Problème de packing?\n", sizeof(t_bmp_info), INFO_HEADER_SIZE);
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("bmp24_saveImage: Erreur ouverture fichier écriture");
        return;
    }

    t_bmp_header file_h_write;
    t_bmp_info info_h_write;

    int image_width = img->width;
    int image_height_abs = abs(img->height);

    // 1. Initialiser t_bmp_header (file_h_write)
    file_h_write.type = BMP_TYPE_SIGNATURE;
    file_h_write.reserved1 = 0;
    file_h_write.reserved2 = 0;
    file_h_write.offset = (uint32_t)(FILE_HEADER_SIZE + INFO_HEADER_SIZE); // 14 + 40 = 54

    // 2. Initialiser t_bmp_info (info_h_write)
    info_h_write.size = INFO_HEADER_SIZE;
    info_h_write.width = image_width;
    info_h_write.height = image_height_abs;
    info_h_write.planes = 1;
    info_h_write.bits_per_pixel = DEFAULT_COLOR_DEPTH_24;
    info_h_write.compression = 0;

    // Valeurs par défaut pour les champs de résolution et couleurs
    info_h_write.x_pixels_per_meter = img->info_header.x_pixels_per_meter != 0 ? img->info_header.x_pixels_per_meter : 2835;
    info_h_write.y_pixels_per_meter = img->info_header.y_pixels_per_meter != 0 ? img->info_header.y_pixels_per_meter : 2835;
    info_h_write.ncolors = 0;
    info_h_write.importantcolors = 0;

    // 3. Calculer les tailles
    uint32_t bytes_per_pixel = info_h_write.bits_per_pixel / 8;
    uint32_t row_data_size = (uint32_t)image_width * bytes_per_pixel;
    uint32_t row_padded_size_write = (row_data_size + 3) & ~3u;

    info_h_write.image_size = row_padded_size_write * (uint32_t)image_height_abs;
    file_h_write.size = file_h_write.offset + info_h_write.image_size;

    // 4. Écrire les headers
    if (fwrite(&file_h_write, sizeof(t_bmp_header), 1, file) != 1) {
        perror("bmp24_saveImage: Erreur écriture t_bmp_header");
        fclose(file); return;
    }
    if (fwrite(&info_h_write, sizeof(t_bmp_info), 1, file) != 1) {
        perror("bmp24_saveImage: Erreur écriture t_bmp_info");
        fclose(file); return;
    }

    // 5. Se positionner pour l'écriture des données pixel
    if (fseek(file, (long)file_h_write.offset, SEEK_SET) != 0) {
        perror("bmp24_saveImage: Erreur fseek vers offset données pixel");
        fclose(file); return;
    }

    uint8_t *row_buffer = (uint8_t *)malloc(row_padded_size_write);
    if (!row_buffer) {
        perror("bmp24_saveImage: Erreur malloc row_buffer");
        fclose(file); return;
    }

    // 6. Écrire les données pixel (bottom-up, BGR)
    for (int i = 0; i < image_height_abs; ++i) {
        int source_y = image_height_abs - 1 - i;

        // Remplir le buffer de ligne avec les données BGR
        for (int x = 0; x < image_width; ++x) {
            row_buffer[x * bytes_per_pixel + 0] = img->data[source_y][x].blue;
            row_buffer[x * bytes_per_pixel + 1] = img->data[source_y][x].green;
            row_buffer[x * bytes_per_pixel + 2] = img->data[source_y][x].red;
        }

        // Remplir le padding avec des zéros
        for (uint32_t k = row_data_size; k < row_padded_size_write; ++k) {
            row_buffer[k] = 0;
        }

        if (fwrite(row_buffer, 1, row_padded_size_write, file) != row_padded_size_write) {
            fprintf(stderr, "bmp24_saveImage: Erreur écriture ligne pixel %d (source_y %d).\n", i, source_y);
            free(row_buffer); fclose(file); return;
        }
    }
    free(row_buffer);

    if (fclose(file) == EOF) {
        perror("bmp24_saveImage: Erreur lors de la fermeture du fichier");
        // L'image est potentiellement corrompue si l'écriture n'a pas été flushée.
        return;
    }
    printf("Image sauvegardée sous '%s'.\n", filename);
}

void bmp24_printInfo(t_bmp24 *img) {
    if (!img) {
        printf("bmp24_printInfo: Image non chargée.\n");
        return;
    }
    printf("--- Informations Image BMP 24 bits ---\n");
    printf("  Fichier Header (t_bmp_header):\n");
    printf("    Type: 0x%X ('%c%c')\n", img->header.type, (img->header.type & 0xFF), (img->header.type >> 8));
    printf("    File Size: %u octets\n", img->header.size);
    printf("    Reserved1: %u\n", img->header.reserved1);
    printf("    Reserved2: %u\n", img->header.reserved2);
    printf("    Pixel Data Offset: %u\n", img->header.offset);
    printf("  Info Header (t_bmp_info):\n");
    printf("    Header Size: %u octets\n", img->info_header.size);
    printf("    Width: %d px\n", img->info_header.width);
    printf("    Height: %d px (%s dans fichier si >0, %s en mémoire)\n",img->info_header.height,(img->info_header.height > 0 ? "bottom-up" : "top-down"),"toujours top-down");
    printf("    Planes: %u\n", img->info_header.planes);
    printf("    Bits/pixel: %u\n", img->info_header.bits_per_pixel);
    printf("    Compression: %u (%s)\n", img->info_header.compression, (img->info_header.compression == 0 ? "BI_RGB" : "Compressé/Autre"));
    printf("    Image Size (données pixel): %u octets\n", img->info_header.image_size);
    printf("    X Pixels/meter: %d\n", img->info_header.x_pixels_per_meter);
    printf("    Y Pixels/meter: %d\n", img->info_header.y_pixels_per_meter);
    printf("    Num Colors in Palette: %u\n", img->info_header.ncolors);
    printf("    Num Important Colors: %u\n", img->info_header.importantcolors);
    printf("  Champs t_bmp24 (internes):\n");
    printf("    img->width (utilisé): %d\n", img->width);
    printf("    img->height (signé, pour info): %d\n", img->height);
    printf("    img->colorDepth (utilisé): %d\n", img->colorDepth);
    printf("--------------------------------------\n");
}

// Traitement d'Image
uint8_t clamp_pixel_value(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

void bmp24_negative(t_bmp24 *img) {
    if (!img || !img->data) return;
    int h = abs(img->height);
    int w = img->width;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            img->data[y][x].red   = 255 - img->data[y][x].red;
            img->data[y][x].green = 255 - img->data[y][x].green;
            img->data[y][x].blue  = 255 - img->data[y][x].blue;
        }
    }
}

void bmp24_grayscale(t_bmp24 *img) {
    if (!img || !img->data) return;
    int h = abs(img->height);
    int w = img->width;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t gray = (uint8_t)(((unsigned int)img->data[y][x].red +
                                      (unsigned int)img->data[y][x].green +
                                      (unsigned int)img->data[y][x].blue) / 3);

            img->data[y][x].red = gray;
            img->data[y][x].green = gray;
            img->data[y][x].blue = gray;
        }
    }
}

void bmp24_brightness(t_bmp24 *img, int value) {
    if (!img || !img->data) return;
    int h = abs(img->height);
    int w = img->width;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            img->data[y][x].red   = clamp_pixel_value((int)img->data[y][x].red + value);
            img->data[y][x].green = clamp_pixel_value((int)img->data[y][x].green + value);
            img->data[y][x].blue  = clamp_pixel_value((int)img->data[y][x].blue + value);
        }
    }
}

void bmp24_threshold(t_bmp24 *img, int threshold_val) {
    if (!img || !img->data) return;
    int h = abs(img->height);
    int w = img->width;
    if (threshold_val < 0) threshold_val = 0;
    if (threshold_val > 255) threshold_val = 255;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t gray = (uint8_t)(((unsigned int)img->data[y][x].red +
                                      (unsigned int)img->data[y][x].green +
                                      (unsigned int)img->data[y][x].blue) / 3);
            uint8_t output_val = (gray >= threshold_val) ? 255 : 0;
            img->data[y][x].red = output_val;
            img->data[y][x].green = output_val;
            img->data[y][x].blue = output_val;
        }
    }
}

//  Filtres de Convolution
void bmp24_apply_filter_generic(t_bmp24 *img, float kernel[3][3], float factor, int bias) {
    if (!img || !img->data) return;
    int h = abs(img->height);
    int w = img->width;

    if (w < 3 || h < 3) {
        fprintf(stderr, "bmp24_apply_filter_generic: Image trop petite (min 3x3 requis) pour appliquer un filtre 3x3.\n");
        return;
    }

    t_pixel **original_data = bmp24_allocateDataPixels(w, h);
    if (!original_data) {
        fprintf(stderr, "bmp24_apply_filter_generic: Erreur allocation copie des données pixel.\n");
        return;
    }

    // Copier les données originales
    for (int y_copy = 0; y_copy < h; ++y_copy) {
        memcpy(original_data[y_copy], img->data[y_copy], (size_t)w * sizeof(t_pixel));
    }

    // Appliquer le filtre
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    t_pixel p = original_data[y + ky][x + kx];
                    float k_val = kernel[ky + 1][kx + 1];
                    sum_r += (float)p.red * k_val;
                    sum_g += (float)p.green * k_val;
                    sum_b += (float)p.blue * k_val;
                }
            }
            img->data[y][x].red   = clamp_pixel_value((int)roundf(sum_r * factor + (float)bias));
            img->data[y][x].green = clamp_pixel_value((int)roundf(sum_g * factor + (float)bias));
            img->data[y][x].blue  = clamp_pixel_value((int)roundf(sum_b * factor + (float)bias));
        }
    }
    bmp24_freeDataPixels(original_data, h);
}

void bmp24_boxBlur(t_bmp24 *img) {
    float k[3][3] = {{1/9.f, 1/9.f, 1/9.f},
                     {1/9.f, 1/9.f, 1/9.f},
                     {1/9.f, 1/9.f, 1/9.f}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}
void bmp24_gaussianBlur(t_bmp24 *img) {
    float k[3][3] = {{1/16.f, 2/16.f, 1/16.f},
                     {2/16.f, 4/16.f, 2/16.f},
                     {1/16.f, 2/16.f, 1/16.f}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}
void bmp24_outline(t_bmp24 *img) {
    float k[3][3] = {{-1, -1, -1},
                     {-1,  8, -1},
                     {-1, -1, -1}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}
void bmp24_emboss(t_bmp24 *img) {
    float k[3][3] = {{-2, -1,  0},
                     {-1,  1,  1},
                     { 0,  1,  2}};
    bmp24_apply_filter_generic(img, k, 1.0f, 128);
}
void bmp24_sharpen(t_bmp24 *img) {
    float k[3][3] = {{ 0, -1,  0},
                     {-1,  5, -1},
                     { 0, -1,  0}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}

// Égalisation d'Histogramme Couleur

// Fonction statique pour convertir un pixel RGB en YUV
static t_yuv convert_rgb_to_yuv(t_pixel rgb) {
    t_yuv yuv;
    float r_float = (float)rgb.red;
    float g_float = (float)rgb.green;
    float b_float = (float)rgb.blue;

    yuv.y = 0.299f * r_float + 0.587f * g_float + 0.114f * b_float;
    yuv.u = -0.14713f * r_float - 0.28886f * g_float + 0.436f * b_float;
    yuv.v = 0.615f * r_float - 0.51499f * g_float - 0.10001f * b_float;

    return yuv;
}

// Fonction statique pour convertir un pixel YUV en RGB
static t_pixel convert_yuv_to_rgb(t_yuv yuv) {
    t_pixel rgb;
    float r_float, g_float, b_float;

    r_float = yuv.y + 1.13983f * yuv.v;
    g_float = yuv.y - 0.39465f * yuv.u - 0.58060f * yuv.v;
    b_float = yuv.y + 2.03211f * yuv.u;

    rgb.red   = clamp_pixel_value((int)roundf(r_float));
    rgb.green = clamp_pixel_value((int)roundf(g_float));
    rgb.blue  = clamp_pixel_value((int)roundf(b_float));

    return rgb;
}

void bmp24_equalize(t_bmp24 *img) {
    if (!img || !img->data) {
        fprintf(stderr, "bmp24_equalize: Image non valide.\n");
        return;
    }

    int width = img->width;
    int height_abs = abs(img->height);
    int total_pixels = width * height_abs;

    if (total_pixels == 0) {
        fprintf(stderr, "bmp24_equalize: Image de taille nulle.\n");
        return;
    }

    //Alloue de la mémoire pour l'image YUV temporaire
    t_yuv **yuv_image_data = (t_yuv **)malloc((size_t)height_abs * sizeof(t_yuv *));
    if (!yuv_image_data) {
        perror("bmp24_equalize: Erreur malloc pour les lignes YUV");
        return;
    }
    for (int i = 0; i < height_abs; ++i) {
        yuv_image_data[i] = (t_yuv *)malloc((size_t)width * sizeof(t_yuv));
        if (!yuv_image_data[i]) {
            perror("bmp24_equalize: Erreur malloc pour une ligne YUV");
            for (int j = 0; j < i; ++j) free(yuv_image_data[j]);
            free(yuv_image_data);
            return;
        }
    }

    //Convertir RGB en YUV
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            yuv_image_data[y][x] = convert_rgb_to_yuv(img->data[y][x]);
        }
    }

    //Calculer l'histogramme de la composante Y
    unsigned int histogram_y[256] = {0};
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            int y_val_int = clamp_pixel_value((int)roundf(yuv_image_data[y][x].y));
            histogram_y[y_val_int]++;
        }
    }

    //Calculer l'histogramme cumulé (CDF) pour Y
    unsigned int cdf_y[256] = {0};
    cdf_y[0] = histogram_y[0];
    for (int i = 1; i < 256; ++i) {
        cdf_y[i] = cdf_y[i - 1] + histogram_y[i];
    }

    // Trouver cdf_min_y
    unsigned int cdf_min_y = 0;
    // Si l'image est complètement noire, cdf_min_y pourrait ne pas être trouvée (rester 0) ou être total_pixels
    // si tous les pixels sont de valeur Y=0.
    for (int i = 0; i < 256; ++i) {
        if (histogram_y[i] > 0) { // On cherche la première intensité qui existe
            cdf_min_y = cdf_y[i];
            break;
        }
    }
    // Si l'image est monochrome (tous les pixels ont la même valeur Y), cdf_min_y sera total_pixels.

    // Créer la LUT (Look-Up Table) pour l'égalisation de Y
    unsigned char lut_y[256];
    float denominator = (float)(total_pixels - cdf_min_y);

    if (denominator <= 0) { // Cas où cdf_min_y >= total_pixels (ex: image monochrome, ou presque)
        for (int i = 0; i < 256; ++i) {
            lut_y[i] = (unsigned char)i; // Ne rien changer
        }
    } else {
        for (int i = 0; i < 256; ++i) {
            if (cdf_y[i] < cdf_min_y) {
                lut_y[i] = 0; // Mapper à la valeur la plus basse
            } else {
                float new_y_float = roundf(((float)cdf_y[i] - cdf_min_y) / denominator * 255.0f);
                lut_y[i] = clamp_pixel_value((int)new_y_float);
            }
        }
    }

    // Appliquer l'égalisation à la composante Y
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            int original_y_int = clamp_pixel_value((int)roundf(yuv_image_data[y][x].y));
            yuv_image_data[y][x].y = (float)lut_y[original_y_int];
        }
    }

    // Convertir l'image YUV (avec Y modifié) de nouveau en RGB
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            img->data[y][x] = convert_yuv_to_rgb(yuv_image_data[y][x]);
        }
    }

    // Libérer la mémoire
    for (int i = 0; i < height_abs; ++i) {
        free(yuv_image_data[i]);
    }
    free(yuv_image_data);

    printf("Égalisation d'histogramme couleur (YUV) appliquée.\n");
}