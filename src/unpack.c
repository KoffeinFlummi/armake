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
#include "utils.h"
#include "unpack.h"


bool is_garbage(struct header *header) {
    int i;
    char c;

    if (header->packing_method != 0)
        return true;

    for (i = 0; i < strlen(header->name); i++) {
        c = header->name[i];
        if (c <= 31)
            return true;
        if (c == '"' ||
                c == '*' ||
                c == ':' ||
                c == '<' ||
                c == '>' ||
                c == '?' ||
                c == '/' ||
                c == '|')
            return true;
    }

    return false;
}


int unpack() {
    extern DocoptArgs args;
    extern int current_operation;
    extern char current_target[2048];
    FILE *f_source;
    FILE *f_target;
    int num_files;
    long i;
    long j;
    long fp_tmp;
    char prefix[2048] = { 0 };
    char full_path[2048];
    char buffer[2048];
    struct header headers[MAXFILES];

    current_operation = OP_UNPACK;
    strcpy(current_target, args.source);

    // remove trailing slash in target
    if (args.target[strlen(args.target) - 1] == PATHSEP)
        args.target[strlen(args.target) - 1] = 0;

    // open file
    f_source = fopen(args.source, "rb");
    if (!f_source) {
        errorf("Failed to open %s.\n", args.source);
        return 1;
    }

    // create folder
    if (create_folders(args.target)) {
        errorf("Failed to create output folder %s.\n", args.target);
        return 2;
    }

    // read header extensions and prefix
    fseek(f_source, 1, SEEK_SET);
    fgets(buffer, 5, f_source);
    if (strncmp(buffer, "sreV", 4) != 0) {
        errorf("Unrecognized file format.\n");
        return 3;
    }

    fseek(f_source, 21, SEEK_SET);
    while (true) {
        fp_tmp = ftell(f_source);
        if (strcmp(buffer, "prefix") == 0) {
            fgets(prefix, sizeof(prefix), f_source);
            fseek(f_source, fp_tmp + strlen(prefix) + 1, SEEK_SET);
            strcpy(buffer, prefix);
        } else {
            fgets(buffer, sizeof(buffer), f_source);
            fseek(f_source, fp_tmp + strlen(buffer) + 1, SEEK_SET);
        }
        if (strlen(buffer) == 0)
            break;
    }

    // read headers
    for (num_files = 0; num_files <= MAXFILES; num_files++) {
        fp_tmp = ftell(f_source);
        fgets(buffer, sizeof(buffer), f_source);
        fseek(f_source, fp_tmp + strlen(buffer) + 1, SEEK_SET);
        if (strlen(buffer) == 0) {
            fseek(f_source, sizeof(uint32_t) * 5, SEEK_CUR);
            break;
        }

        strcpy(headers[num_files].name, buffer);
        fread(&headers[num_files].packing_method, sizeof(uint32_t), 1, f_source);
        fread(&headers[num_files].original_size, sizeof(uint32_t), 1, f_source);
        fseek(f_source, sizeof(uint32_t) * 2, SEEK_CUR);
        fread(&headers[num_files].data_size, sizeof(uint32_t), 1, f_source);
    }
    if (num_files > MAXFILES) {
        errorf("Maximum number of files (%i) exceeded.\n", MAXFILES);
        return 4;
    }

    // read files
    for (i = 0; i < num_files; i++) {
        // check for garbage
        if (is_garbage(&headers[i])) {
            fseek(f_source, headers[i].data_size, SEEK_CUR);
            continue;
        }

        // replace pathseps on linux
#ifndef _WIN32
        for (j = 0; j < strlen(headers[i].name); j++) {
            if (headers[i].name[j] == '\\')
                headers[i].name[j] = PATHSEP;
        }
#endif

        // get full path
        strcpy(full_path, args.target);
        strcat(full_path, "?");
        full_path[strlen(full_path) - 1] = PATHSEP;
        strcat(full_path, headers[i].name);

        // create containing folder
        strcpy(buffer, full_path);
        if (strrchr(buffer, PATHSEP) != NULL) {
            *strrchr(buffer, PATHSEP) = 0;
            if (create_folders(buffer)) {
                errorf("Failed to create folder %s.\n", buffer);
                return 5;
            }
        }

        // open target file
        if (access(full_path, F_OK) != -1 && !args.force) {
            errorf("File %s already exists and --force was not set.\n", full_path);
            return 6;
        }
        f_target = fopen(full_path, "wb");
        if (!f_target) {
            errorf("Failed to open file %s.\n", full_path);
            return 7;
        }

        // write to file
        for (j = 0; headers[i].data_size - j >= sizeof(buffer); j += sizeof(buffer)) {
            fread(buffer, sizeof(buffer), 1, f_source);
            fwrite(buffer, sizeof(buffer), 1, f_target);
        }
        fread(buffer, headers[i].data_size - j, 1, f_source);
        fwrite(buffer, headers[i].data_size - j, 1, f_target);

        // clean up
        fclose(f_target);
    }

    // clean up
    fclose(f_source);

    // if prefix file wasn't included but there is a prefix, create one
    strcpy(full_path, args.target);
    strcat(full_path, "?");
    full_path[strlen(full_path) - 1] = PATHSEP;
    strcat(full_path, "$PBOPREFIX$");
    if (access(full_path, F_OK) == -1 && strlen(prefix) > 0) {
        f_target = fopen(full_path, "wb");
        if (!f_target) {
            errorf("Failed to open file %s.\n", full_path);
            return 8;
        }

        fputs(prefix, f_target);
        fputc('\n', f_target);

        fclose(f_target);
    }

    return 0;
}
