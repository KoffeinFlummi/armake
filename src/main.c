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


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "docopt.h"
#include "binarize.h"
#include "build.h"

#define VERSION "v1.0"


int main(int argc, char *argv[]) {
    extern DocoptArgs args;
    extern char muted_warnings[MAXWARNINGS][512];
    int i;
    int j;
    char *halp[] = {argv[0], "-h"};

    args = docopt(argc, argv, 1, VERSION);

    // Docopt doesn't yet support positional arguments
    if (argc < 4)
        docopt(2, halp, 1, VERSION);

    args.source = argv[argc - 2];
    args.target = argv[argc - 1];

    if (args.source[0] == '-' || args.target[0] == '-')
        docopt(2, halp, 1, VERSION);


    // @todo
    for (i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--include") == 0)
            args.includefolder = argv[i+1];
        if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0) {
            for (j = 0; j < MAXWARNINGS && muted_warnings[j][0] != 0; j++);
            strncpy(muted_warnings[j], argv[i + 1], sizeof(muted_warnings[j]));
        }
    }


    if (args.binarize)
        return binarize();
    if (args.build)
        return build();
    if (args.unpack)
        return unpack();


    docopt(2, halp, 1, VERSION);
    return 1;
}
