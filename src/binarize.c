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

#include "docopt.h"
#include "filesystem.h"


int mlod2odol(char *source, char *target, char *tempfolder) {
    return 0;
}


int rapify_file(char *source, char *target, char *tempfolder, char *includefolders) {
    return 0;
}


int binarize_file(char *source, char *target, char *tempfolder, char *includefolders) {
    /*
     * Binarize the given file. If source and target are identical, the target
     * is overwritten. If the source is a P3D, it is converted to ODOL. If the
     * source is a rapifiable type (cpp, rvmat, etc.), it is rapified.
     *
     * If the file type is not recognized, -1 is returned.
     *
     * The include folders argument is the paths to be searched for file
     * inclusions, seperated by colons (:).
     */

    // Get file type
    char fileext[64];
    strncpy(fileext, strrchr(source, '.'), 64);

    if (!strcmp(fileext, ".cpp") ||
            !strcmp(fileext, ".rvmat") ||
            !strcmp(fileext, ".ext"))
        return rapify_file(source, target, tempfolder, includefolders);
    if (!strcmp(fileext, ".p3d"))
        return mlod2odol(source, target, tempfolder);

    return -1;
}


int binarize(DocoptArgs args) {
    // check if target already exists
    FILE *f_target;
    if (access(args.target, F_OK) != -1 && !args.force) {
        printf("File %s already exists and --force was not set.\n", args.target);
        return 1;
    }

    // remove trailing slash in source and target
    if (args.source[strlen(args.source) - 1] == PATHSEP)
        args.source[strlen(args.source) - 1] = 0;
    if (args.target[strlen(args.target) - 1] == PATHSEP)
        args.target[strlen(args.target) - 1] = 0;

    char tempfolder[2048];
    create_temp_folder("", tempfolder, 2048);

    char *includefolders = ".";
    if (args.include)
        includefolders = args.includefolders;

    int success = binarize_file(args.source, args.target, tempfolder, includefolders);

    if (success == -1) {
        printf("File is no P3D and doesn't seem rapifiable.\n");
        return 1;
    }

    return success;
}
