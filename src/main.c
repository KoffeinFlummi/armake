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

#include "docopt.h"
#include "img2paa.h"
#include "paa2img.h"
#include "build.h"

#define VERSION "v1.0"


int main(int argc, char *argv[]) {
    DocoptArgs args = docopt(argc, argv, 1, VERSION);
    char *halp[] = {argv[0], "-h"};

    // Docopt doesn't yet support positional arguments
    int j = 0;
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') { continue; }
        if (j < 2) {
            j++;
            continue;
        }
        if (j == 2) {
            args.source = argv[i];
            j++;
            continue;
        }
        if (j == 3) {
            args.target = argv[i];
            j++;
            continue;
        }
        docopt(2, halp, 1, VERSION);
    }

    if (args.img2paa)
        return img2paa(args);
    if (args.paa2img)
        return paa2img(args);
    if (args.build)
        return build(args);

    docopt(2, halp, 1, VERSION);
}
