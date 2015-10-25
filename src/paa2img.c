/*
 * Copyright (C)  2015  Felix "KoffeinFlummi" Wiegand
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "minilzo.h"

#include "docopt.h"
#include "paa2img.h"


int dxt12img(unsigned char *input, unsigned char *output, int width, int height) {
    /* Convert DXT1 data into a PNG image array. */

    typedef struct dxt1block {
        uint16_t c0 : 16;
        uint16_t c1 : 16;
        uint8_t cl3 : 2;
        uint8_t cl2 : 2;
        uint8_t cl1 : 2;
        uint8_t cl0 : 2;
        uint8_t cl7 : 2;
        uint8_t cl6 : 2;
        uint8_t cl5 : 2;
        uint8_t cl4 : 2;
        uint8_t cl11 : 2;
        uint8_t cl10 : 2;
        uint8_t cl9 : 2;
        uint8_t cl8 : 2;
        uint8_t cl15 : 2;
        uint8_t cl14 : 2;
        uint8_t cl13 : 2;
        uint8_t cl12 : 2;
    } dxt1block_t;

    for (int i = 0; i < (width * height) / 2; i += 8) {
        dxt1block_t block;
        memcpy(&block, input + i, 8);

        uint8_t c[4][3] = {
            {
                255 * ((63488 & block.c0) >> 11) / 31,
                255 * ((2016 & block.c0) >> 5) / 63,
                255 * (31 & block.c0) / 31
            },
            {
                255 * ((63488 & block.c1) >> 11) / 31,
                255 * ((2016 & block.c1) >> 5) / 63,
                255 * (31 & block.c1) / 31
            }
        };
        c[2][0] = (2 * c[0][0] + 1 * c[1][0]) / 3;
        c[2][1] = (2 * c[0][1] + 1 * c[1][1]) / 3;
        c[2][2] = (2 * c[0][2] + 1 * c[1][2]) / 3;
        c[3][0] = (1 * c[0][0] + 2 * c[1][0]) / 3;
        c[3][1] = (1 * c[0][1] + 2 * c[1][1]) / 3;
        c[3][2] = (1 * c[0][2] + 2 * c[1][2]) / 3;

        unsigned int clookup[16] = {
            block.cl0, block.cl1, block.cl2, block.cl3,
            block.cl4, block.cl5, block.cl6, block.cl7,
            block.cl8, block.cl9, block.cl10, block.cl11,
            block.cl12, block.cl13, block.cl14, block.cl15
        };

        unsigned int x, y, index;
        for (int j = 0; j < 16; j++) {
            x = ((i / 8) % (width / 4)) * 4 + 3 - (j % 4);
            y = ((i / 8) / (height / 4)) * 4 + (j / 4);
            index = (y * width + x) * 4;
            char pixel[4] = {
                c[clookup[j]][0],
                c[clookup[j]][1],
                c[clookup[j]][2],
                255
            };
            memcpy(output + index, pixel, 4);
        }
    }

    return 0;
}


int dxt32img(unsigned char *input, unsigned char *output, int width, int height) {
    /* Convert DXT3 data into a PNG image array. */

    typedef struct dx35block {
        uint8_t a3 : 4;
        uint8_t a2 : 4;
        uint8_t a1 : 4;
        uint8_t a0 : 4;
        uint8_t a7 : 4;
        uint8_t a6 : 4;
        uint8_t a5 : 4;
        uint8_t a4 : 4;
        uint8_t a11 : 4;
        uint8_t a10 : 4;
        uint8_t a9 : 4;
        uint8_t a8 : 4;
        uint8_t a15 : 4;
        uint8_t a14 : 4;
        uint8_t a13 : 4;
        uint8_t a12 : 4;
        uint16_t c0 : 16;
        uint16_t c1 : 16;
        uint8_t cl3 : 2;
        uint8_t cl2 : 2;
        uint8_t cl1 : 2;
        uint8_t cl0 : 2;
        uint8_t cl7 : 2;
        uint8_t cl6 : 2;
        uint8_t cl5 : 2;
        uint8_t cl4 : 2;
        uint8_t cl11 : 2;
        uint8_t cl10 : 2;
        uint8_t cl9 : 2;
        uint8_t cl8 : 2;
        uint8_t cl15 : 2;
        uint8_t cl14 : 2;
        uint8_t cl13 : 2;
        uint8_t cl12 : 2;
    } dxt3block_t;

    for (int i = 0; i < width * height; i += 16) {
        dxt3block_t block;
        memcpy(&block, input + i, 16);

        unsigned int a[16] = {
            block.a0, block.a1, block.a2, block.a3,
            block.a4, block.a5, block.a6, block.a7,
            block.a8, block.a9, block.a10, block.a11,
            block.a12, block.a13, block.a14, block.a15
        };

        uint8_t c[4][3] = {
            {
                255 * ((63488 & block.c0) >> 11) / 31,
                255 * ((2016 & block.c0) >> 5) / 63,
                255 * (31 & block.c0) / 31
            },
            {
                255 * ((63488 & block.c1) >> 11) / 31,
                255 * ((2016 & block.c1) >> 5) / 63,
                255 * (31 & block.c1) / 31
            }
        };
        c[2][0] = (2 * c[0][0] + 1 * c[1][0]) / 3;
        c[2][1] = (2 * c[0][1] + 1 * c[1][1]) / 3;
        c[2][2] = (2 * c[0][2] + 1 * c[1][2]) / 3;
        c[3][0] = (1 * c[0][0] + 2 * c[1][0]) / 3;
        c[3][1] = (1 * c[0][1] + 2 * c[1][1]) / 3;
        c[3][2] = (1 * c[0][2] + 2 * c[1][2]) / 3;

        unsigned int clookup[16] = {
            block.cl0, block.cl1, block.cl2, block.cl3,
            block.cl4, block.cl5, block.cl6, block.cl7,
            block.cl8, block.cl9, block.cl10, block.cl11,
            block.cl12, block.cl13, block.cl14, block.cl15
        };

        unsigned int x, y, index;
        for (int j = 0; j < 16; j++) {
            x = ((i / 16) % (width / 4)) * 4 + 3 - (j % 4);
            y = ((i / 16) / (height / 4)) * 4 + (j / 4);
            index = (y * width + x) * 4;
            char pixel[4] = {
                c[clookup[j]][0],
                c[clookup[j]][1],
                c[clookup[j]][2],
                a[j]
            };
            memcpy(output + index, pixel, 4);
        }
    }

    return 0;
}


int dxt52img(unsigned char *input, unsigned char *output, int width, int height) {
    /* Convert DXT5 data into a PNG image array. */

    /* For some reason, directly unpacking the alpha lookup table into the 16
     * 3-bit arrays didn't work, so i'm reading it into one 64bit integer and
     * unpacking it manually later. @todo */
    typedef struct dxt5block {
        uint8_t a0 : 8;
        uint8_t a1 : 8;
        uint64_t al : 48;
        uint16_t c0 : 16;
        uint16_t c1 : 16;
        uint8_t cl3 : 2;
        uint8_t cl2 : 2;
        uint8_t cl1 : 2;
        uint8_t cl0 : 2;
        uint8_t cl7 : 2;
        uint8_t cl6 : 2;
        uint8_t cl5 : 2;
        uint8_t cl4 : 2;
        uint8_t cl11 : 2;
        uint8_t cl10 : 2;
        uint8_t cl9 : 2;
        uint8_t cl8 : 2;
        uint8_t cl15 : 2;
        uint8_t cl14 : 2;
        uint8_t cl13 : 2;
        uint8_t cl12 : 2;
    } dxt5block_t;

    for (int i = 0; i < width * height; i += 16) {
        dxt5block_t block;
        memcpy(&block, input + i, 16);

        uint8_t a[8] = {block.a0, block.a1};
        if (block.a0 > block.a1) { // @todo, this shouldn't be necessary
            a[2] = (6 * block.a0 + 1 * block.a1) / 7;
            a[3] = (5 * block.a0 + 2 * block.a1) / 7;
            a[4] = (4 * block.a0 + 3 * block.a1) / 7;
            a[5] = (3 * block.a0 + 4 * block.a1) / 7;
            a[6] = (2 * block.a0 + 5 * block.a1) / 7;
            a[7] = (1 * block.a0 + 6 * block.a1) / 7;
        } else {
            a[2] = (4 * block.a0 + 1 * block.a1) / 5;
            a[3] = (3 * block.a0 + 2 * block.a1) / 5;
            a[4] = (2 * block.a0 + 3 * block.a1) / 5;
            a[5] = (1 * block.a0 + 4 * block.a1) / 5;
            a[6] = 0;
            a[7] = 255;
        }

        // This is ugly, retarded and shouldn't be necessary. See above.
        unsigned int alookup[16] = {
            (block.al & 3584) >> 9,
            (block.al & 448) >> 6,
            (block.al & 56) >> 3,
            (block.al & 7) >> 0,
            (block.al & 14680064) >> 21,
            (block.al & 1835008) >> 18,
            (block.al & 229376) >> 15,
            (block.al & 28672) >> 12,
            (block.al & 60129542144) >> 33,
            (block.al & 7516192768) >> 30,
            (block.al & 939524096) >> 27,
            (block.al & 117440512) >> 24,
            (block.al & 246290604621824) >> 45,
            (block.al & 30786325577728) >> 42,
            (block.al & 3848290697216) >> 39,
            (block.al & 481036337152) >> 36
        };

        uint8_t c[4][3] = {
            {
                255 * ((63488 & block.c0) >> 11) / 31,
                255 * ((2016 & block.c0) >> 5) / 63,
                255 * (31 & block.c0) / 31
            },
            {
                255 * ((63488 & block.c1) >> 11) / 31,
                255 * ((2016 & block.c1) >> 5) / 63,
                255 * (31 & block.c1) / 31
            }
        };
        c[2][0] = (2 * c[0][0] + 1 * c[1][0]) / 3;
        c[2][1] = (2 * c[0][1] + 1 * c[1][1]) / 3;
        c[2][2] = (2 * c[0][2] + 1 * c[1][2]) / 3;
        c[3][0] = (1 * c[0][0] + 2 * c[1][0]) / 3;
        c[3][1] = (1 * c[0][1] + 2 * c[1][1]) / 3;
        c[3][2] = (1 * c[0][2] + 2 * c[1][2]) / 3;

        unsigned int clookup[16] = {
            block.cl0, block.cl1, block.cl2, block.cl3,
            block.cl4, block.cl5, block.cl6, block.cl7,
            block.cl8, block.cl9, block.cl10, block.cl11,
            block.cl12, block.cl13, block.cl14, block.cl15
        };

        unsigned int x, y, index;
        for (int j = 0; j < 16; j++) {
            x = ((i / 16) % (width / 4)) * 4 + 3 - (j % 4);
            y = ((i / 16) / (height / 4)) * 4 + (j / 4);
            index = (y * width + x) * 4;
            char pixel[4] = {
                c[clookup[j]][0],
                c[clookup[j]][1],
                c[clookup[j]][2],
                a[alookup[j]]
            };
            memcpy(output + index, pixel, 4);
        }
    }

    return 0;
}


int paa2img(DocoptArgs args) {
    // @todo: implement force

    printf("Converting PAA to image ...\n\n");

    FILE *f = fopen(args.source, "rb");
    if (!f) {
        printf("Couldn't open source file.");
        return 2;
    }

    uint16_t paatype;
    fread(&paatype, 2, 1, f);

    switch (paatype) {
        case DXT1:     printf("PAA is type DXT1.\n\n"); break;
        case DXT3:     printf("PAA is type DXT3.\n\n"); break;
        case DXT5:     printf("PAA is type DXT5.\n\n"); break;
        case RGBA4444: printf("PAA is type RGBA4444.\n\n"); break;
        case RGBA5551: printf("PAA is type RGBA5551.\n\n"); break;
        case GRAY:     printf("PAA is type GRAY.\n\n"); break;
        default:       printf("Unrecognized PAA type.\n\n"); return 3;
    }

    char taggsig[4];
    char taggname[4];
    uint32_t tagglen;
    uint32_t mipmap;
    printf("Finding MIPMAP pointer ... ");
    while (1) {
        fread(taggsig, 4, 1, f);
        taggsig[4] = 0x00;
        if (strcmp(taggsig, "GGAT")) { printf("failed.\n"); return 4; }

        fread(taggname, 4, 1, f);
        taggname[4] = 0x00;
        fread(&tagglen, 4, 1, f);
        if (strcmp(taggname, "SFFO")) {
            fseek(f, tagglen, SEEK_CUR);
            continue;
        }

        fread(&mipmap, 4, 1, f);
        break;
    }
    printf("done.\n");

    printf("Reading first MIPMAP ...\n");
    uint16_t width;
    uint16_t height;
    uint24_t datalen;
    fseek(f, mipmap, SEEK_SET);
    fread(&width, 2, 1, f);
    fread(&height, 2, 1, f);
    fread(&datalen, 3, 1, f);

    unsigned char compressed[datalen.i];
    fread(compressed, datalen.i, 1, f);
    fclose(f);

    int compression = COMP_NONE;
    if (width % 32768 != width && (paatype == DXT1 || paatype == DXT3 || paatype == DXT5)) {
        width -= 32768;
        compression = COMP_LZO;
        printf("  Compression: LZO\n");
    } else if (paatype == RGBA4444 || paatype == RGBA5551 || paatype == GRAY) {
        compression = COMP_LZSS;
        printf("  Compression: LZSS\n");
    } else {
        printf("  Compression: None\n");
    }
    printf("  Dimensions: %i x %i\n", width, height);

    int imgdatalen = width * height;
    if (paatype == DXT1)
        imgdatalen /= 2;
    unsigned char *imgdata = malloc(imgdatalen);
    if (!imgdata) {
        printf("Failed to allocate enough memory for decompression.\n");
        return 4;
    }
    if (compression == COMP_LZO) {
        printf("Decompressing ... ");
        if (lzo_init() != LZO_E_OK) {
            printf("Failed to init LZO decompressor - this shouldn't be happening ...\n");
            return 5;
        }
        lzo_uint in_len = sizeof(compressed);
        lzo_uint out_len = imgdatalen;
        if (paatype == DXT1) { out_len /= 2; }
        if (lzo1x_decompress(compressed,in_len,imgdata,&out_len,NULL) != LZO_E_OK) {
            printf("Failed to decompress LZO data.\n");
            return 6;
        }
        printf("done.\n");
    } else if (compression == COMP_LZSS) {
        printf("Decompressing ... ");
        // @todo: LZSS compression
        memcpy(imgdata, compressed, imgdatalen);
        printf("done.\n");
    } else {
        memcpy(imgdata, compressed, imgdatalen);
    }

    unsigned char *outputdata = malloc(width * height * 4);
    if (!outputdata) {
        printf("Failed to allocate enough memory for decoding.\n");
        return 7;
    }
    printf("Decoding ... ");
    if (paatype == DXT1) {
        if (dxt12img(imgdata, outputdata, width, height)) {
            printf("failed.\n");
            return 8;
        }
    } else if (paatype == DXT3) {
        if (dxt32img(imgdata, outputdata, width, height)) {
            printf("failed.\n");
            return 8;
        }
    } else if (paatype == DXT5) {
        if (dxt52img(imgdata, outputdata, width, height)) {
            printf("failed.\n");
            return 8;
        }
    } else if (paatype == RGBA4444) {
    } else if (paatype == RGBA5551) {
    } else if (paatype == GRAY) {
    }
    // @todo: decoding for RGBA{4444,5551} and GRAY
    printf("done.\n");

    free(imgdata);

    printf("Writing to file ... ");
    if (!stbi_write_png(args.target, width, height, 4, outputdata, width * 4)) {
        printf("failed.\n");
        return 9;
    }
    printf("done.\n");

    free(outputdata);

    return 0;
}
