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

#include "img2paa.h"
#include "paa2img.h"


#define USAGE "flummitools - Cause I suck at names.\n"\
    "\n"\
    "Usage:\n"\
    "    flummitools [command] [options]\n"\
    "    flummitools [command] (-h | --help)\n"\
    "    flummitools (-h | --help)\n"\
    "    flummitools (-v | --version)\n"\
    "\n"\
    "Commands:\n"\
    "    img2paa      Convert image to PAA\n"\
    "    paa2img      Convert PAA to image\n"\
    "    binarize     Binarize a file\n"\
    "    debinarize   Debinarize a file\n"\
    "    pack         Packs a folder into a PBO (without any binarization)\n"\
    "    build        Binarize and pack an addon folder\n"\
    "\n"\
    "Options:\n"\
    "    -h --help      Show usage information and exit\n"\
    "    -v --version   Print the version number and exit\n"
#define VERSION "v1.0"


int main(int argc, char *argv[]) {
    typedef struct args {
        char *source;
        char *target;
        int force;
        int verbose;
    } args_t;

    if (argc == 1) {
        printf("%s\n", USAGE);
        return 1;
    }
    if (argc == 2 && (strcmp(argv[1], "-h") || strcmp(argv[1], "--help"))) {
        printf("%s\n", USAGE);
        return 0;
    }
    if (argc == 2 && (strcmp(argv[1], "-v") || strcmp(argv[1], "--version"))) {
        printf("%s\n", VERSION);
        return 0;
    }

    args_t args;
    args.source = "";
    args.target = "";
    args.force = 0;
    args.verbose = 0;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "-f") == 0 || strcmp(arg, "--force") == 0) {
            args.force = 1;
            continue;
        }
        if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            args.verbose = 1;
            continue;
        }
        if (strlen(args.source) == 0) {
            args.source = arg;
            continue;
        }
        if (strlen(args.target) == 0) {
            args.target = arg;
            continue;
        }
        printf("%s\n", USAGE);
        return 1;
    }

    if (strlen(args.target) == 0 || strlen(args.source) < 5) {
        printf("%s\n", USAGE);
        return 1;
    }

    char *extension = args.source + strlen(args.source) - 4;

    if (strcmp(extension, ".paa") == 0) {
        return paa2img(args.source, args.target, args.force);
    } else {
        return img2paa(args.source, args.target, args.force);
    }
}
