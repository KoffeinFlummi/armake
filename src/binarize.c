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
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <errno.h>
#include <fts.h>
#endif

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"
#include "rapify.h"
#include "p3d.h"
#include "binarize.h"


int binarize_file(char *source, char *target) {
    /*
     * Binarize the given file. If source and target are identical, the target
     * is overwritten. If the source is a P3D, it is converted to ODOL. If the
     * source is a rapifiable type (cpp, ext, etc.), it is rapified.
     *
     * If the file type is not recognized, -1 is returned. 0 is returned on
     * success and a positive integer on error.
     *
     * The include folder argument is the path to be searched for file
     * inclusions.
     */

    char fileext[64];

    if (strchr(source, '.') == NULL)
        return -1;

    strncpy(fileext, strrchr(source, '.'), 64);

    if (!strcmp(fileext, ".cpp") ||
            !strcmp(fileext, ".ext"))
        return rapify_file(source, target);
    if (!strcmp(fileext, ".p3d"))
        return mlod2odol(source, target);

    return -1;
}


int binarize() {
    extern DocoptArgs args;

    // check if target already exists
    if (access(args.target, F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", args.target);
        return 1;
    }

    int success = binarize_file(args.source, args.target);

    if (success == -1) {
        errorf("File is no P3D and doesn't seem rapifiable.\n");
        return 1;
    }

    return success;
}
