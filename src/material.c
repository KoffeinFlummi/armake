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
#include <math.h>

#include "docopt.h"
#include "filesystem.h"
#include "rapify.h"
#include "preprocess.h"
#include "utils.h"
#include "derapify.h"
#include "material.h"


int read_material(struct material *material) {
    /*
     * Reads the material information for the given material struct.
     * Returns 0 on success and a positive integer on failure.
     */

    extern int current_operation;
    extern char current_target[2048];
    FILE *f;
    char actual_path[2048];
    char rapified_path[2048];

    current_operation = OP_MATERIAL;
    strcpy(current_target, material->path);

    if (find_file(material->path, "", actual_path, NULL)) {
        errorf("Failed to find material %s.\n", material->path);
        return 1;
    }

    strcpy(rapified_path, material->path);
    strcat(rapified_path, ".armake.bin"); // it is assumed that this doesn't exist

    // Rapify file
    if (rapify_file(actual_path, rapified_path)) {
        errorf("Failed to rapify %s.\n", actual_path);
        return 2;
    }

    current_operation = OP_MATERIAL;
    strcpy(current_target, material->path);

    // Open rapified file
    f = fopen(rapified_path, "r");
    if (!f) {
        errorf("Failed to open rapified material.\n");
        return 3;
    }

    material->type = MATERIALTYPE;

    // Clean up
    fclose(f);
    if (remove_file(rapified_path)) {
        errorf("Failed to remove temporary material.\n");
        return 4;
    }

    return 0;
}
