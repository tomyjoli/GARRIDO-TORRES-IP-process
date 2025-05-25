#include "bmp24.h" // DOIT être inclus en premier
#include <string.h> // Pour memcpy
#include <stdio.h>  // Pour perror, fprintf, stderr, FILE, fopen, fclose, fread, fwrite, fseek
#include <stdlib.h> // Pour malloc, free, abs, calloc
#include <stdint.h> // Pour les types entiers
#include <math.h>   // Pour roundf

// Implémentation des fonctions d'aide file_rawRead et file_rawWrite
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
            fprintf(stderr, "file_rawRead: Fin de fichier atteinte prématurément.\n");
        } else if (ferror(file)) {
            perror("file_rawRead: Erreur fread");
        } else {
            // Ne rien afficher ici car cela peut être normal pour les données pixel si image_size est 0
        }
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

// ----- Fonctions d'Allocation et de Libération (PDF Section 2.3) -----
t_pixel **bmp24_allocateDataPixels(int width, int height_abs) {
    if (width <= 0 || height_abs <= 0) {
        fprintf(stderr, "bmp24_allocateDataPixels: Dimensions invalides (%d x %d).\n", width, height_abs);
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
            for (int j = 0; j < i; ++j) free(pixels[j]);
            free(pixels);
            return NULL;
        }
    }
    return pixels;
}

void bmp24_freeDataPixels(t_pixel **pixels, int height_abs) {
    if (!pixels || height_abs <= 0) return;
    for (int i = 0; i < height_abs; ++i) {
        if (pixels[i]) {
            free(pixels[i]);
            pixels[i] = NULL;
        }
    }
    free(pixels);
}

t_bmp24 *bmp24_allocate(int width, int height, int colorDepth) {
    if (width <= 0 || abs(height) <= 0) { // Utiliser abs(height) pour la vérification
        fprintf(stderr, "bmp24_allocate: Dimensions invalides (W:%d x H:%d).\n", width, height);
        return NULL;
    }
    if (colorDepth != DEFAULT_COLOR_DEPTH_24) {
        fprintf(stderr, "bmp24_allocate: Profondeur de couleur non supportée (%d), attendu %d.\n", colorDepth, DEFAULT_COLOR_DEPTH_24);
    }

    t_bmp24 *img = (t_bmp24 *)malloc(sizeof(t_bmp24));
    if (!img) {
        perror("bmp24_allocate: Erreur malloc pour t_bmp24");
        return NULL;
    }

    img->data = bmp24_allocateDataPixels(width, abs(height)); // Toujours allouer avec hauteur positive
    if (!img->data) {
        free(img);
        return NULL;
    }

    img->width = width;
    img->height = height; // Conserver le signe original de la hauteur
    img->colorDepth = colorDepth;

    img->header.type = BMP_TYPE_SIGNATURE;
    img->header.reserved1 = 0;
    img->header.reserved2 = 0;
    img->header.offset = (uint32_t)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

    img->info_header.size = INFO_HEADER_SIZE;
    img->info_header.width = width;
    img->info_header.height = height; // Conserver le signe original
    img->info_header.planes = 1;
    img->info_header.bits_per_pixel = (uint16_t)colorDepth;
    img->info_header.compression = 0;
    uint32_t row_stride_bytes = ((uint32_t)width * 3 + 3) & ~3u;
    img->info_header.image_size = row_stride_bytes * (uint32_t)abs(height);
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

// ----- Lecture et Écriture d'Image (PDF Section 2.4) -----
t_bmp24 *bmp24_loadImage(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("bmp24_loadImage: Erreur ouverture fichier");
        return NULL;
    }

    t_bmp_header file_h_read;
    t_bmp_info info_h_read;

    // Lire t_bmp_header en utilisant file_rawRead
    // Le premier argument est l'offset depuis le début du fichier.
    // Pour t_bmp_header, c'est 0 (ou BITMAP_MAGIC_OFFSET si vous l'utilisez, qui est 0).
    file_rawRead(0, &file_h_read, sizeof(t_bmp_header), 1, file);
    // ATTENTION: file_rawRead tel que défini dans le PDF ne retourne pas de code d'erreur.
    // Vous devrez vérifier ferror(file) ou feof(file) après, ou valider les données lues.
    // Par exemple, vérifier file_h_read.type après la lecture.
    if (ferror(file) || feof(file)) { // Vérification basique
        perror("bmp24_loadImage: Erreur ou EOF après file_rawRead pour t_bmp_header");
        fclose(file);
        return NULL;
    }

    if (file_h_read.type != BMP_TYPE_SIGNATURE) {
        fprintf(stderr, "bmp24_loadImage: Signature BMP invalide.\n"); fclose(file); return NULL;
    }
    printf("DEBUG: Valeur lue pour info_h_read.bits_per_pixel = %u\n", info_h_read.+);
    if (info_h_read.bits_per_pixel != DEFAULT_COLOR_DEPTH_24) {
        fprintf(stderr, "bmp24_loadImage: Image non 24-bits.\n"); fclose(file); return NULL;
    }
    if (info_h_read.compression != 0) {
        fprintf(stderr, "bmp24_loadImage: Compression non supportée.\n"); fclose(file); return NULL;
    }
    if (info_h_read.size != INFO_HEADER_SIZE) {
        fprintf(stderr, "bmp24_loadImage: Avertissement - Taille DIB header anormale.\n");
    }

    t_bmp24 *img = bmp24_allocate(info_h_read.width, info_h_read.height, info_h_read.bits_per_pixel);
    if (!img) {
        fclose(file); return NULL;
    }

    img->header = file_h_read; // Copier les headers lus
    img->info_header = info_h_read;
    // Les width/height/colorDepth de img sont déjà initialisés par bmp24_allocate
    // et correspondent aux valeurs de info_h_read.

    if (fseek(file, (long)img->header.offset, SEEK_SET) != 0) {
        perror("bmp24_loadImage: Erreur fseek données pixel"); bmp24_free(img); fclose(file); return NULL;
    }

    uint32_t row_padded_size = ((uint32_t)img->width * 3 + 3) & ~3u;
    uint8_t *row_buffer = (uint8_t *)malloc(row_padded_size);
    if (!row_buffer) {
        perror("bmp24_loadImage: Erreur malloc row_buffer"); bmp24_free(img); fclose(file); return NULL;
    }

    int h_abs = abs(img->height); // Utiliser la hauteur absolue pour itérer sur les données
    for (int i = 0; i < h_abs; ++i) {
        if (fread(row_buffer, 1, row_padded_size, file) != row_padded_size) {
            fprintf(stderr, "bmp24_loadImage: Erreur lecture ligne pixel %d.\n", i);
            free(row_buffer); bmp24_free(img); fclose(file); return NULL;
        }
        // Si height > 0, fichier est bottom-up, on stocke top-down.
        // Si height < 0, fichier est top-down, on stocke top-down.
        int storage_y = (img->info_header.height > 0) ? (h_abs - 1 - i) : i;
        for (int x = 0; x < img->width; ++x) {
            img->data[storage_y][x].blue  = row_buffer[x * 3 + 0];
            img->data[storage_y][x].green = row_buffer[x * 3 + 1];
            img->data[storage_y][x].red   = row_buffer[x * 3 + 2];
        }
    }
    free(row_buffer);
    fclose(file);
    return img;
}

void bmp24_saveImage(const char *filename, t_bmp24 *img) {
    if (!img || !img->data) { fprintf(stderr, "bmp24_saveImage: Image ou données invalides.\n"); return; }
    FILE *file = fopen(filename, "wb");
    if (!file) { perror("bmp24_saveImage: Erreur ouverture fichier écriture"); return; }

    t_bmp_header file_h_write = img->header;
    t_bmp_info info_h_write = img->info_header;
    // Standard BMP: hauteur positive pour bottom-up. Notre data est top-down.
    info_h_write.height = abs(img->info_header.height);
    // Recalculer image_size et file_size pour être sûr
    uint32_t row_padded_size_write = ((uint32_t)info_h_write.width * 3 + 3) & ~3u;
    info_h_write.image_size = row_padded_size_write * (uint32_t)info_h_write.height;
    file_h_write.size = file_h_write.offset + info_h_write.image_size;

    if (fwrite(&file_h_write, sizeof(t_bmp_header), 1, file) != 1) {
        perror("bmp24_saveImage: Erreur écriture t_bmp_header"); fclose(file); return;
    }
    if (fwrite(&info_h_write, sizeof(t_bmp_info), 1, file) != 1) {
        perror("bmp24_saveImage: Erreur écriture t_bmp_info"); fclose(file); return;
    }
    if (fseek(file, (long)file_h_write.offset, SEEK_SET) != 0) {
        perror("bmp24_saveImage: Erreur fseek offset données pixel"); fclose(file); return;
    }

    uint8_t *row_buffer = (uint8_t *)malloc(row_padded_size_write);
    if (!row_buffer) { perror("bmp24_saveImage: Erreur malloc row_buffer"); fclose(file); return; }

    int h_abs = abs(img->height); // Hauteur des données en mémoire (toujours positive pour l'itération)
    for (int i = 0; i < h_abs; ++i) {
        // Écrire en bottom-up: la ligne i du fichier correspond à la ligne (h_abs - 1 - i) de nos données top-down.
        int source_y = h_abs - 1 - i;
        for (int x = 0; x < img->width; ++x) {
            row_buffer[x * 3 + 0] = img->data[source_y][x].blue;
            row_buffer[x * 3 + 1] = img->data[source_y][x].green;
            row_buffer[x * 3 + 2] = img->data[source_y][x].red;
        }
        for (uint32_t k = (uint32_t)img->width * 3; k < row_padded_size_write; ++k) row_buffer[k] = 0;

        if (fwrite(row_buffer, 1, row_padded_size_write, file) != row_padded_size_write) {
            fprintf(stderr, "bmp24_saveImage: Erreur écriture ligne pixel %d.\n", i);
            free(row_buffer); fclose(file); return;
        }
    }
    free(row_buffer);
    fclose(file);
    printf("Image sauvegardée sous '%s'.\n", filename);
}

void bmp24_printInfo(t_bmp24 *img) {
    if (!img) { printf("bmp24_printInfo: Image non chargée.\n"); return; }
    printf("--- Informations Image BMP 24 bits (Structure PDF) ---\n");
    printf("  Fichier Header (t_bmp_header):\n");
    printf("    Type: 0x%X ('%c%c')\n", img->header.type, (img->header.type & 0xFF), (img->header.type >> 8));
    printf("    Size: %u octets\n", img->header.size);
    printf("    Offset données: %u\n", img->header.offset);
    printf("  Info Header (t_bmp_info):\n");
    printf("    Size: %u octets\n", img->info_header.size);
    printf("    Width: %d px\n", img->info_header.width);
    printf("    Height: %d px (%s)\n", img->info_header.height, (img->info_header.height > 0 ? "bottom-up dans fichier" : "top-down dans fichier"));
    printf("    Planes: %u\n", img->info_header.planes);
    printf("    Bits/pixel: %u\n", img->info_header.bits_per_pixel);
    printf("    Compression: %u\n", img->info_header.compression);
    printf("    Image Size: %u octets\n", img->info_header.image_size);
    printf("  Champs t_bmp24 (internes):\n");
    printf("    img->width: %d\n", img->width);
    printf("    img->height: %d\n", img->height);
    printf("    img->colorDepth: %d\n", img->colorDepth);
    printf("-----------------------------------------------------\n");
}

// ----- Traitement d'Image (PDF Section 2.5) -----
uint8_t clamp_pixel_value(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

void bmp24_negative(t_bmp24 *img) {
    if (!img || !img->data) return;
    int h = abs(img->height); int w = img->width;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            img->data[y][x].red   = 255 - img->data[y][x].red;
            img->data[y][x].green = 255 - img->data[y][x].green;
            img->data[y][x].blue  = 255 - img->data[y][x].blue;
        }
}

void bmp24_grayscale(t_bmp24 *img) {
    if (!img || !img->data) return;
    int h = abs(img->height); int w = img->width;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            uint8_t gray = (uint8_t)(((unsigned int)img->data[y][x].red +
                                      (unsigned int)img->data[y][x].green +
                                      (unsigned int)img->data[y][x].blue) / 3);
            img->data[y][x].red = gray; img->data[y][x].green = gray; img->data[y][x].blue = gray;
        }
}

void bmp24_brightness(t_bmp24 *img, int value) {
    if (!img || !img->data) return;
    int h = abs(img->height); int w = img->width;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            img->data[y][x].red   = clamp_pixel_value((int)img->data[y][x].red + value);
            img->data[y][x].green = clamp_pixel_value((int)img->data[y][x].green + value);
            img->data[y][x].blue  = clamp_pixel_value((int)img->data[y][x].blue + value);
        }
}

void bmp24_threshold(t_bmp24 *img, int threshold_val) {
    if (!img || !img->data) return;
    int h = abs(img->height); int w = img->width;
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
            uint8_t gray = (uint8_t)(((unsigned int)img->data[y][x].red +
                                      (unsigned int)img->data[y][x].green +
                                      (unsigned int)img->data[y][x].blue) / 3);
            uint8_t output_val = (gray >= threshold_val) ? 255 : 0;
            img->data[y][x].red = output_val; img->data[y][x].green = output_val; img->data[y][x].blue = output_val;
        }
}

// ----- Filtres de Convolution (PDF Section 2.6) -----
void bmp24_apply_filter_generic(t_bmp24 *img, float kernel[3][3], float factor, int bias) {
    if (!img || !img->data) return;
    int h = abs(img->height); int w = img->width;
    if (w < 3 || h < 3) { fprintf(stderr, "Image trop petite pour kernel 3x3.\n"); return; }

    t_pixel **original_data = bmp24_allocateDataPixels(w, h);
    if (!original_data) return;
    for (int y_copy = 0; y_copy < h; ++y_copy) for (int x_copy = 0; x_copy < w; ++x_copy) {
            original_data[y_copy][x_copy] = img->data[y_copy][x_copy];
        }

    for (int y = 1; y < h - 1; ++y) for (int x = 1; x < w - 1; ++x) {
            float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
            for (int ky = -1; ky <= 1; ++ky) for (int kx = -1; kx <= 1; ++kx) {
                    t_pixel p = original_data[y + ky][x + kx]; float k_val = kernel[ky + 1][kx + 1];
                    sum_r += (float)p.red * k_val; sum_g += (float)p.green * k_val; sum_b += (float)p.blue * k_val;
                }
            img->data[y][x].red   = clamp_pixel_value((int)roundf(sum_r * factor + bias));
            img->data[y][x].green = clamp_pixel_value((int)roundf(sum_g * factor + bias));
            img->data[y][x].blue  = clamp_pixel_value((int)roundf(sum_b * factor + bias));
        }
    bmp24_freeDataPixels(original_data, h);
}

void bmp24_boxBlur(t_bmp24 *img) {
    float k[3][3] = {{1/9.f,1/9.f,1/9.f},{1/9.f,1/9.f,1/9.f},{1/9.f,1/9.f,1/9.f}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}
void bmp24_gaussianBlur(t_bmp24 *img) {
    float k[3][3] = {{1/16.f,2/16.f,1/16.f},{2/16.f,4/16.f,2/16.f},{1/16.f,2/16.f,1/16.f}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}
void bmp24_outline(t_bmp24 *img) {
    float k[3][3] = {{-1,-1,-1},{-1,8,-1},{-1,-1,-1}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}
void bmp24_emboss(t_bmp24 *img) {
    float k[3][3] = {{-2,-1,0},{-1,1,1},{0,1,2}};
    bmp24_apply_filter_generic(img, k, 1.0f, 128);
}
void bmp24_sharpen(t_bmp24 *img) {
    float k[3][3] = {{0,-1,0},{-1,5,-1},{0,-1,0}};
    bmp24_apply_filter_generic(img, k, 1.0f, 0);
}
// ----- NOUVEAU pour la Partie 3 : Égalisation d'Histogramme Couleur -----

// Fonction statique pour convertir un pixel RGB en YUV (Formule 3.3, PDF page 31)
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

// Fonction statique pour convertir un pixel YUV en RGB (Formule 3.4, PDF page 31)
// Assure le clamping des valeurs RGB dans [0, 255]
static t_pixel convert_yuv_to_rgb(t_yuv yuv) {
    t_pixel rgb;
    float r_float, g_float, b_float;

    r_float = yuv.y + 1.13983f * yuv.v;
    g_float = yuv.y - 0.39465f * yuv.u - 0.58060f * yuv.v;
    b_float = yuv.y + 2.03211f * yuv.u;

    // Clamper et arrondir (comme suggéré page 32 "convertir les valeurs flottantes en entiers")
    // et "s'assurer que les valeurs ... restent dans les limites appropriées (0-255)"
    rgb.red   = clamp_pixel_value((int)roundf(r_float));
    rgb.green = clamp_pixel_value((int)roundf(g_float));
    rgb.blue  = clamp_pixel_value((int)roundf(b_float));

    return rgb;
}

// Fonction principale pour l'égalisation d'histogramme couleur (PDF Section 3.4.3)
void bmp24_equalize(t_bmp24 *img) {
    if (!img || !img->data) {
        fprintf(stderr, "bmp24_equalize: Image non valide.\n");
        return;
    }

    int width = img->width;
    int height_abs = abs(img->height); // Toujours travailler avec une hauteur positive pour les boucles
    int total_pixels = width * height_abs;

    if (total_pixels == 0) {
        fprintf(stderr, "bmp24_equalize: Image de taille nulle.\n");
        return;
    }

    // Étape 1: Convertir RGB en YUV et stocker les pixels YUV
    // Allouer de la mémoire pour l'image YUV temporaire
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

    // Conversion RGB -> YUV
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            yuv_image_data[y][x] = convert_rgb_to_yuv(img->data[y][x]);
        }
    }

    // Étape 2: Calculer l'histogramme de la composante Y
    unsigned int histogram_y[256] = {0}; // Initialiser à zéro
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            // Arrondir Y et clamper pour l'histogramme
            int y_val_int = clamp_pixel_value((int)roundf(yuv_image_data[y][x].y));
            histogram_y[y_val_int]++;
        }
    }

    // Étape 3: Calculer l'histogramme cumulé (CDF) et normaliser pour la composante Y
    unsigned int cdf_y[256] = {0};
    cdf_y[0] = histogram_y[0];
    for (int i = 1; i < 256; ++i) {
        cdf_y[i] = cdf_y[i - 1] + histogram_y[i];
    }

    // Trouver cdf_min_y (première valeur non nulle dans la CDF)
    // C'est la CDF de la plus basse intensité Y présente.
    unsigned int cdf_min_y = 0;
    for (int i = 0; i < 256; ++i) {
        if (cdf_y[i] > 0) { // Le PDF dit "plus petite valeur non nulle de l'histogramme cumulé CDF"
            // ce qui implique que cdf_min_y peut être la cdf_y[i] pour le premier i où hist[i]>0
            cdf_min_y = cdf_y[i]; // Si le premier hist[i] > 0 est à l'index k, cdf_min_y sera cdf_y[k]
            break;
        }
    }
    // Si toutes les valeurs de cdf_y sont 0 (image complètement noire par ex.),
    // alors cdf_min_y restera 0.

    // Créer la LUT (Look-Up Table) pour l'égalisation de Y (Formule 3.2)
    unsigned char lut_y[256];
    float N_minus_cdf_min_y = (float)(total_pixels - cdf_min_y);

    if (N_minus_cdf_min_y <= 0) { // Éviter division par zéro ou si toutes les valeurs sont >= cdf_min_y
        // Cas particulier: image avec très peu de variations ou toutes les valeurs Y sont identiques
        // après la première intensité Y non nulle. On peut choisir de ne rien faire ou de tout mapper à une valeur.
        // Pour simplifier, si la plage dynamique est nulle ou négative, on ne change pas Y.
        for (int i = 0; i < 256; ++i) {
            lut_y[i] = (unsigned char)i;
        }
    } else {
        for (int i = 0; i < 256; ++i) {
            if (cdf_y[i] < cdf_min_y) { // Pour les intensités Y en dessous de la première significative
                lut_y[i] = 0; // Ou la valeur mappée de cdf_min_y
            } else {
                float new_y_float = roundf(((float)cdf_y[i] - cdf_min_y) / N_minus_cdf_min_y * 255.0f);
                lut_y[i] = clamp_pixel_value((int)new_y_float);
            }
        }
    }

    // Étape 4: Appliquer l'égalisation à la composante Y (dans notre image YUV temporaire)
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            int original_y_int = clamp_pixel_value((int)roundf(yuv_image_data[y][x].y));
            // La nouvelle valeur Y est une valeur entière de la LUT, mais on la stocke comme float
            // car U et V sont aussi des floats et la reconversion YUV->RGB attend des floats.
            yuv_image_data[y][x].y = (float)lut_y[original_y_int];
            // U et V restent inchangés
        }
    }

    // Étape 5: Convertir l'image YUV (avec Y modifié) de nouveau en RGB
    for (int y = 0; y < height_abs; ++y) {
        for (int x = 0; x < width; ++x) {
            img->data[y][x] = convert_yuv_to_rgb(yuv_image_data[y][x]);
        }
    }

    // Libérer la mémoire allouée pour l'image YUV temporaire
    for (int i = 0; i < height_abs; ++i) {
        free(yuv_image_data[i]);
    }
    free(yuv_image_data);

    printf("Égalisation d'histogramme couleur (YUV) appliquée.\n");
}