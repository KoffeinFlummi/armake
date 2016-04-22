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

#include "filesystem.h"
#include "rapify.h"
#include "utils.h"
#include "model_config.h"


// Once derap is attempted, these should probably be moved there

int skip_array(FILE *f) {
    int i;
    uint8_t type;
    uint32_t num_entries;

    num_entries = read_compressed_int(f);

    for (i = 0; i < num_entries; i++) {
        type = fgetc(f);

        if (type == 0 || type == 4)
            while (fgetc(f) != 0);
        else if (type == 1 || type == 2)
            fseek(f, 4, SEEK_CUR);
        else
            skip_array(f);
    }

    return 0;
}

int seek_config_path(FILE *f, char *config_path) {
    /*
     * Assumes the file pointer f points at the start of a rapified
     * class body. The config path should be formatted like one used
     * by the ingame commands (case insensitive):
     *
     *   CfgExample >> MyClass >> MyValue
     *
     * This function then moves the file pointer to the start of the
     * desired class entry (or class body for classes).
     *
     * Returns a positive integer on failure, a 0 on success and -1
     * if the given path doesn't exist.
     */

    int i;
    uint8_t type;
    uint32_t num_entries;
    uint32_t fp;
    char path[2048];
    char target[512];
    char buffer[512];

    // Trim leading spaces
    strncpy(path, config_path, sizeof(path));
    trim_leading(path, sizeof(path));

    // Extract first element
    for (i = 0; i < sizeof(target) - 1; i++) {
        if (path[i] == 0 || path[i] == '>' || path[i] == ' ')
            break;
    }
    strncpy(target, path, i);
    target[i] = 0;
    lower_case(target);

    // Inherited classname
    while (fgetc(f) != 0);

    num_entries = read_compressed_int(f);

    for (i = 0; i < num_entries; i++) {
        fp = ftell(f);
        type = fgetc(f);

        if (type == 0) { // class
            if (fgets(buffer, sizeof(buffer), f) == NULL)
                return 1;

            lower_case(buffer);
            fseek(f, fp + strlen(buffer) + 2, SEEK_SET);
            fread(&fp, 4, 1, f);

            if (strcmp(buffer, target))
                continue;

            fseek(f, fp, SEEK_SET);

            if (strchr(path, '>') == NULL)
                return 0;

            strcpy(path, strchr(config_path, '>') + 2);
            return seek_config_path(f, path);
        } else if (type == 1) { // value
            type = fgetc(f);

            if (fgets(buffer, sizeof(buffer), f) == NULL)
                return 1;

            lower_case(buffer);

            if (strcmp(buffer, target) == 0) {
                fseek(f, fp, SEEK_SET);
                return 0;
            }

            fseek(f, fp + strlen(buffer) + 3, SEEK_SET);

            if (type == 0 || type == 4)
                while (fgetc(f) != 0);
            else
                fseek(f, 4, SEEK_CUR);
        } else if (type == 2) { // array
            if (fgets(buffer, sizeof(buffer), f) == NULL)
                return 1;

            lower_case(buffer);

            if (strcmp(buffer, target) == 0) {
                fseek(f, fp, SEEK_SET);
                return 0;
            }

            fseek(f, fp + strlen(buffer) + 2, SEEK_SET);

            skip_array(f);
        } else { // extern & delete statements
            while (fgetc(f) != 0);
        }
    }

    return -1;
}


int seek_parent(FILE *f) {
    /*
     * Assumes the file pointer f points at the start of a rapified
     * class body. This function then moves the pointer to the parent
     * of the current class.
     *
     * Returns -1 if the class doesn't have a parent class, -2 if that
     * class cannot be found, 0 on success and a positive integer
     * on failure.
     *
     * If -1 or -2 are returned, the file pointer is not moved.
     */

    return 0;
}


int seek_definition(FILE *f, char *config_path) {
    /*
     * Finds the definition of the given value, even if it is defined in a
     * parent class.
     *
     * Returns 0 on success, a positive integer on failure.
     */

    int success;

    fseek(f, 16, SEEK_SET);

    // Try the direct way first
    success = seek_config_path(f, config_path);
    if (success >= 0)
        return success;

    // Try to find the definition @todo

    return 0;
}


int read_string(FILE *f, char *config_path, char *buffer, size_t buffsize) {
    /*
     * Reads the given config string into the given buffer.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    int success;
    long fp;
    uint8_t temp;

    success = seek_definition(f, config_path);
    if (success > 0)
        return success;

    temp = fgetc(f);
    if (temp != 1)
        return 1;

    temp = fgetc(f);
    if (temp != 0)
        return 2;

    while (fgetc(f) != 0);

    fp = ftell(f);

    if (fgets(buffer, buffsize, f) == NULL)
        return 3;

    fseek(f, fp + strlen(buffer) + 1, SEEK_SET);

    return 0;
}


int read_int(FILE *f, char *config_path, uint32_t *result) {
    /*
     * Reads the given integer from config.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    int success;
    uint8_t temp;

    success = seek_definition(f, config_path);
    if (success > 0)
        return success;

    temp = fgetc(f);
    if (temp != 1)
        return 1;

    temp = fgetc(f);
    if (temp != 2)
        return 2;

    while (fgetc(f) != 0);

    fread(result, 4, 1, f);

    return 0;
}


int read_float(FILE *f, char *config_path, float *result) {
    /*
     * Reads the given float from config.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    int success;
    uint8_t temp;

    success = seek_definition(f, config_path);
    if (success > 0)
        return success;

    temp = fgetc(f);
    if (temp != 1)
        return 1;

    temp = fgetc(f);
    if (temp != 1)
        return 2;

    while (fgetc(f) != 0);

    fread(result, 4, 1, f);

    return 0;
}


int read_array(FILE *f, char *config_path, char *buffer, int size, size_t buffsize) {
    /*
     * Reads the given array from config. size should be the maximum number of
     * elements in the array, buffsize the length of the individual buffers.
     *
     * At the moment, only one-dimensional string arrays are supported.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    int i;
    int success;
    long fp;
    uint8_t temp;
    uint32_t num_entries;

    success = seek_definition(f, config_path);
    if (success > 0)
        return success;

    temp = fgetc(f);
    if (temp != 2)
        return 1;

    while (fgetc(f) != 0);

    num_entries = read_compressed_int(f);

    for (i = 0; i < num_entries; i++) {
        // Array is full
        if (i == size)
            return 2;

        temp = fgetc(f);
        if (temp != 0)
            return 3;

        fp = ftell(f);

        if (fgets(buffer + i * buffsize, buffsize, f) == NULL)
            return 3;

        fseek(f, fp + strlen(buffer + i * buffsize) + 1, SEEK_SET);
    }

    return 0;
}


int read_classes(FILE *f, char *config_path, char *buffer, int size, size_t buffsize) {
    return 0;
}


int read_model_config(char *path, struct skeleton *skeleton) {
    /*
     * Reads the model config information for the given model path. If no
     * model config is found, -1 is returned. 0 is returned on success
     * and a positive integer on failure.
     */

    FILE *f;
    int i;
    int success;
    char model_config_path[2048];
    char rapified_path[2048];
    char config_path[2048];
    char model_name[512];
    char bones[MAXBONES * 2][512];
    char buffer[512];

    // Extract model.cfg path
    strncpy(model_config_path, path, sizeof(model_config_path));
    if (strrchr(model_config_path, PATHSEP) != NULL) {
        strcpy(strrchr(model_config_path, PATHSEP), "?model.cfg");
        *strrchr(model_config_path, '?') = PATHSEP;
    } else {
        strcpy(model_config_path, "model.cfg");
    }

    strcpy(rapified_path, model_config_path);
    strcat(rapified_path, ".armake.bin"); // it is assumed that this doesn't exist

    if (access(model_config_path, F_OK) == -1)
        return -1;

    // Rapify file
    success = rapify_file(model_config_path, rapified_path, ".");
    if (success) {
        printf("Failed to rapify model config.\n");
        return 1;
    }

    // Extract model name and convert to lower case
    if (strrchr(path, PATHSEP) != NULL)
        strcpy(model_name, strrchr(path, PATHSEP) + 1);
    else
        strcpy(model_name, path);
    *strrchr(model_name, '.') = 0;

    lower_case(model_name);

    // Open rapified file
    f = fopen(rapified_path, "r");
    if (!f) {
        printf("Failed to open model config.\n");
        return 2;
    }

    // Read name
    sprintf(config_path, "CfgModels >> %s >> skeletonName", model_name);
    success = read_string(f, config_path, skeleton->name, sizeof(skeleton->name));
    if (success) {
        printf("Failed to read skeleton name.\n");
        return success;
    }

    // Read bones
    sprintf(config_path, "CfgSkeletons >> %s >> skeletonInherit", model_name);
    success = read_string(f, config_path, buffer, sizeof(buffer));
    if (success) {
        printf("Failed to read bones.\n");
        return success;
    }

    i = 0;
    if (strlen(buffer) > 0) {
        sprintf(config_path, "CfgSkeletons >> %s >> skeletonBones", buffer);
        success = read_array(f, config_path, (char *)bones, MAXBONES * 2, 512);
        if (success) {
            printf("Failed to read bones.\n");
            return success;
        }
        for (i = 0; i < MAXBONES * 2; i += 2) {
            if (bones[i][0] == 0)
                break;
        }
    }

    sprintf(config_path, "CfgSkeletons >> %s >> skeletonBones", model_name);
    success = read_array(f, config_path, (char *)bones + i * 512, MAXBONES * 2 - i, 512);
    if (success) {
        printf("Failed to read bones.\n");
        return success;
    }

    for (i = 0; i < MAXBONES * 2; i += 2) {
        if (bones[i][0] == 0)
            break;
        strcpy(skeleton->bones[i / 2].name, bones[i]);
        strcpy(skeleton->bones[i / 2].parent, bones[i + 1]);
        skeleton->num_bones++;
    }

    // Read sections
    sprintf(config_path, "CfgModels >> %s >> sectionsInherit", model_name);
    success = read_string(f, config_path, buffer, sizeof(buffer));
    if (success) {
        printf("Failed to read sections.\n");
        return success;
    }

    i = 0;
    if (strlen(buffer) > 0) {
        sprintf(config_path, "CfgModels >> %s >> sections", buffer);
        success = read_array(f, config_path, (char *)skeleton->sections, MAXSECTIONS, 512);
        if (success) {
            printf("Failed to read sections.\n");
            return success;
        }
        for (i = 0; i < MAXSECTIONS; i++) {
            if (skeleton->sections[i][0] == 0)
                break;
        }
    }

    sprintf(config_path, "CfgModels >> %s >> sections", model_name);
    success = read_array(f, config_path, (char *)skeleton->sections + i * 512, MAXSECTIONS - i, 512);
    if (success) {
        printf("Failed to read sections.\n");
        return success;
    }

    for (i = 0; i < MAXSECTIONS; i++) {
        if (skeleton->sections[i][0] == 0)
            break;
        skeleton->num_sections++;
    }


    // Clean up @debug
    fclose(f);
    if (remove_file(rapified_path)) {
        printf("Failed to remove temporary model config.\n");
        return 3;
    }

    return 0;
}
