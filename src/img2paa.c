/*
 * Copyright (C)  2016  Felix "KoffeinFlummi" Wiegand
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


#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"
#include "minilzo.h"

#include "docopt.h"
#include "utils.h"
#include "paa2img.h"
#include "img2paa.h"


int img2dxt1(unsigned char *input, unsigned char *output, int width, int height) {
    /*
     * Converts image data to DXT1 data.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    unsigned char img_block[64];
    unsigned char dxt_block[8];
    int i;
    int j;

    for (i = 0; i < width; i += 4) {
        for (j = 0; j < height; j += 4) {
            memcpy(img_block +  0, input + (i + 0) * width * 4 + j * 4, 16);
            memcpy(img_block + 16, input + (i + 1) * width * 4 + j * 4, 16);
            memcpy(img_block + 32, input + (i + 2) * width * 4 + j * 4, 16);
            memcpy(img_block + 48, input + (i + 3) * width * 4 + j * 4, 16);

            stb_compress_dxt_block(dxt_block, (const unsigned char *)img_block, 0, STB_DXT_HIGHQUAL);

            memcpy(output + (i / 4) * (width / 4) * sizeof(dxt_block) + (j / 4) * sizeof(dxt_block), dxt_block, sizeof(dxt_block));
        }
    }

    return 0;
}


int img2dxt5(unsigned char *input, unsigned char *output, int width, int height) {
    /*
     * Converts image data to DXT5 data.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    unsigned char img_block[64];
    unsigned char dxt_block[16];
    int i;
    int j;

    for (i = 0; i < width; i += 4) {
        for (j = 0; j < height; j += 4) {
            memcpy(img_block +  0, input + (i + 0) * width * 4 + j * 4, 16);
            memcpy(img_block + 16, input + (i + 1) * width * 4 + j * 4, 16);
            memcpy(img_block + 32, input + (i + 2) * width * 4 + j * 4, 16);
            memcpy(img_block + 48, input + (i + 3) * width * 4 + j * 4, 16);

            stb_compress_dxt_block(dxt_block, (const unsigned char *)img_block, 1, STB_DXT_HIGHQUAL);

            memcpy(output + (i / 4) * (width / 4) * sizeof(dxt_block) + (j / 4) * sizeof(dxt_block), dxt_block, sizeof(dxt_block));
        }
    }

    return 0;
}


int calculate_average_color(unsigned char *imgdata, int num_pixels, unsigned char color[4]) {
    uint32_t total_color[4];
    int i;
    int j;

    memset(total_color, 0, 16);

    for (i = 0; i < num_pixels; i++) {
        for (j = 0; j < 4; j++)
            total_color[j] += (uint32_t)imgdata[i * 4 + j];
    }

    for (i = 0; i < 4; i++)
        color[i] = (unsigned char)(total_color[i ^ 2] / num_pixels);

    return 0;
}


int calculate_maximum_color(unsigned char *imgdata, int num_pixels, unsigned char color[4]) {
    int i;
    int j;

    memset(color, 0, 4);

    for (i = 0; i < num_pixels; i++) {
        for (j = 0; j < 4; j++) {
            if (imgdata[(i ^ 2) * 4 + j] > color[j])
                color[j] = imgdata[(i ^ 2) * 4 + j];
        }
    }

    return 0;
}


int img2paa(char *source, char *target) {
    /*
     * Converts source image to target PAA.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    extern DocoptArgs args;

    FILE *f_target;
    uint32_t offsets[16];
    uint32_t datalen;
    uint16_t paatype;
    uint16_t width;
    uint16_t height;
    long fp_offsets;
    int num_channels;
    int w;
    int h;
    int i;
    lzo_uint in_len;
    lzo_uint out_len;
    bool compressed;
    unsigned char *imgdata;
    unsigned char *tmp;
    unsigned char *workmem;
    unsigned char *outputdata;
    unsigned char color[4];
    
    if (!args.paatype) {
        paatype = 0;
    } else if (stricmp("DXT1", args.paatype) == 0) {
        paatype = DXT1;
    } else if (stricmp("DXT3", args.paatype) == 0) {
        errorf("DXT3 support is not implemented.\n");
        return 4;
    } else if (stricmp("DXT5", args.paatype) == 0) {
        paatype = DXT5;
    } else if (stricmp("ARGB4444", args.paatype) == 0) {
        errorf("ARGB4444 support is not implemented.\n");
        return 4;
    } else if (stricmp("ARGB1555", args.paatype) == 0) {
        errorf("ARGB1555 support is not implemented.\n");
        return 4;
    } else if (stricmp("AI88", args.paatype) == 0) {
        errorf("AI88 support is not implemented.\n");
        return 4;
    } else {
        errorf("Unrecognized PAA type \"%s\".\n", args.paatype);
        return 4;
    }

    imgdata = stbi_load(source, &w, &h, &num_channels, 4);
    if (!imgdata) {
        errorf("Failed to load image.\n");
        return 1;
    }

    width = w;
    height = h;

    // Check if alpha channel is necessary
    if (num_channels == 4) {
        num_channels--;
        for (i = 3; i < width * height * 4; i += 4) {
            if (imgdata[i] < 0xff) {
                num_channels++;
                break;
            }
        }
    }

    // Unless told otherwise, use DXT5 for alpha stuff and DXT1 for everything else
    if (paatype == 0) {
        paatype = (num_channels == 4) ? DXT5 : DXT1;
    }

    if (width % 4 != 0 || height % 4 != 0) {
        errorf("Dimensions are no multiple of 4.\n");
        stbi_image_free(imgdata);
        return 2;
    }

    tmp = (unsigned char *)malloc(width * height * 4);
    memcpy(tmp, imgdata, width * height * 4);
    stbi_image_free(imgdata);
    imgdata = tmp;

    f_target = fopen(target, "wb");
    if (!f_target) {
        errorf("Failed to open target file.\n");
        free(imgdata);
        return 3;
    }

    // Type
    fwrite(&paatype, sizeof(paatype), 1, f_target);

    // TAGGs
    fwrite("GGATCGVA", 8, 1, f_target);
    fwrite("\x04\x00\x00\x00", 4, 1, f_target);
    calculate_average_color(imgdata, width * height, color);
    fwrite(color, sizeof(color), 1, f_target);

    fwrite("GGATCXAM", 8, 1, f_target);
    fwrite("\x04\x00\x00\x00", 4, 1, f_target);
    calculate_maximum_color(imgdata, width * height, color);
    fwrite(color, sizeof(color), 1, f_target);

    fwrite("GGATSFFO", 8, 1, f_target);
    fwrite("\x40\x00\x00\x00", 4, 1, f_target);
    fp_offsets = ftell(f_target);
    memset(offsets, 0, sizeof(offsets));
    fwrite(offsets, sizeof(offsets), 1, f_target);

    // Palette
    fwrite("\x00\x00", 2, 1, f_target);

    // MipMaps
    for (i = 0; i < 15; i++) {
        datalen = width * height;
        if (paatype == DXT1)
            datalen /= 2;

        outputdata = (unsigned char *)malloc(datalen);

        // Convert to output format
        switch (paatype) {
            case DXT1:
                if (img2dxt1(imgdata, outputdata, width, height)) {
                    errorf("Failed to convert image data to DXT1.\n");
                    free(outputdata);
                    free(imgdata);
                    return 5;
                }
                break;
            case DXT5:
                if (img2dxt5(imgdata, outputdata, width, height)) {
                    errorf("Failed to convert image data to DXT5.\n");
                    free(outputdata);
                    free(imgdata);
                    return 5;
                }
                break;
            default:
                free(outputdata);
                free(imgdata);
                return 5;
        }

        // LZO compression
        compressed = args.compress && datalen > LZO1X_MEM_COMPRESS;

        if (compressed) {
            tmp = (unsigned char *)malloc(datalen);
            workmem = (unsigned char *)malloc(LZO1X_MEM_COMPRESS);
            in_len = datalen;

            memcpy(tmp, outputdata, datalen);

            if (lzo_init() != LZO_E_OK) {
                errorf("Failed to initialize LZO for compression.\n");
                free(workmem);
                free(tmp);
                free(outputdata);
                free(imgdata);
                return 6;
	    }
            if (lzo1x_1_compress(tmp, in_len, outputdata, &out_len, workmem) != LZO_E_OK) {
                errorf("Failed to compress image data.\n");
                free(workmem);
                free(tmp);
                free(outputdata);
                free(imgdata);
                return 6;
            }

            free(workmem);
            free(tmp);

            datalen = out_len;
        }

        // Write to file
        offsets[i] = ftell(f_target);
        if (compressed)
            width += 32768;
        fwrite(&width, sizeof(width), 1, f_target);
        if (compressed)
            width -= 32768;
        fwrite(&height, sizeof(height), 1, f_target);
        fwrite(&datalen, 3, 1, f_target);
        fwrite(outputdata, datalen, 1, f_target);

        free(outputdata);

        // Resize image for next MipMap
        width /= 2;
        height /= 2;

        if (width < 4 || height < 4) { break; }

        tmp = (unsigned char *)malloc(width * height * 4);
        if (!stbir_resize_uint8(imgdata, width * 2, height * 2, 0, tmp, width, height, 0, 4)) {
            errorf("Failed to resize image.\n");
            free(tmp);
            free(imgdata);
            return 7;
        }
        free(imgdata);
        imgdata = tmp;
    }

    offsets[i] = ftell(f_target);
    fwrite("\x00\x00\x00\x00", 4, 1, f_target);

    fwrite("\x00\x00", 2, 1, f_target);

    // Update offsets
    fseek(f_target, fp_offsets, SEEK_SET);
    fwrite(offsets, sizeof(offsets), 1, f_target);

    fclose(f_target);
    free(imgdata);
    
    return 0;
}


int cmd_img2paa() {
    extern DocoptArgs args;

    // check if target already exists
    if (strcmp(args.target, "-") != 0 && access(args.target, F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", args.target);
        return 1;
    }

    return img2paa(args.source, args.target);
}
