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
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "args.h"
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


int cmd_inspect() {
    extern struct arguments args;
    extern char *current_target;
    FILE *f_target;
    int num_files;
    long i;
    long fp_tmp;
    char buffer[2048];
    bool reading_headerext;
    struct header *headers;

    if (args.num_positionals != 2)
        return 128;

    headers = (struct header *)safe_malloc(sizeof(struct header) * MAXFILES);

    current_target = args.positionals[1];

    // remove trailing slash in target
    if (args.positionals[1][strlen(args.positionals[1]) - 1] == PATHSEP)
        args.positionals[1][strlen(args.positionals[1]) - 1] = 0;

    // open file
    f_target = fopen(args.positionals[1], "rb");
    if (!f_target) {
        errorf("Failed to open %s.\n", args.positionals[1]);
        free(headers);
        return 1;
    }

    // read header extensions
    fseek(f_target, 1, SEEK_SET);
    fgets(buffer, 5, f_target);
    if (strncmp(buffer, "sreV", 4) == 0) {
        fseek(f_target, 21, SEEK_SET);
        printf("Header extensions:\n");
        i = 0;
        reading_headerext = false;
        while (true) {
            fp_tmp = ftell(f_target);
            buffer[i++] = fgetc(f_target);
            buffer[i] = '\0';
            if (buffer[i - 1] == '\0') {
                if (buffer[i - 2] == '\0') {
                    break;
                } else {
                    if (reading_headerext) {
                        printf("%s\n", buffer);
                        reading_headerext = false;
                    } else {
                        printf("- %s=", buffer);
                        reading_headerext = true;
                    }
                    i = 0;
                }
            }
        }
        printf("\n");
    } else {
        fseek(f_target, 0, SEEK_SET);
    }

    // read headers
    for (num_files = 0; num_files <= MAXFILES; num_files++) {
        fp_tmp = ftell(f_target);
        fgets(buffer, sizeof(buffer), f_target);
        fseek(f_target, fp_tmp + strlen(buffer) + 1, SEEK_SET);
        if (strlen(buffer) == 0) {
            fseek(f_target, sizeof(uint32_t) * 5, SEEK_CUR);
            break;
        }

        strcpy(headers[num_files].name, buffer);
        fread(&headers[num_files].packing_method, sizeof(uint32_t), 1, f_target);
        fread(&headers[num_files].original_size, sizeof(uint32_t), 1, f_target);
        fseek(f_target, sizeof(uint32_t) * 2, SEEK_CUR);
        fread(&headers[num_files].data_size, sizeof(uint32_t), 1, f_target);
    }
    if (num_files > MAXFILES) {
        errorf("Maximum number of files (%i) exceeded.\n", MAXFILES);
        fclose(f_target);
        free(headers);
        return 4;
    }

    printf("# Files: %i\n\n", num_files);

    printf("Path                                                  Method  Original    Packed\n");
    printf("                                                                  Size      Size\n");
    printf("================================================================================\n");
    for (i = 0; i < num_files; i++) {
        if (headers[i].original_size == 0)
            headers[i].original_size = headers[i].data_size;
        printf("%-50s %9u %9u %9u\n", headers[i].name, headers[i].packing_method, headers[i].original_size, headers[i].data_size);
    }

    // clean up
    fclose(f_target);
    free(headers);

    return 0;
}


int cmd_unpack() {
    extern struct arguments args;
    extern char *current_target;
    FILE *f_source;
    FILE *f_target;
    int num_files;
    long i;
    long j;
    long fp_tmp;
    char full_path[2048];
    char buffer[2048];
    bool reading_headerext;
    struct header *headers;

    if (args.num_positionals < 3)
        return 128;

    headers = (struct header *)safe_malloc(sizeof(struct header) * MAXFILES);

    current_target = args.positionals[1];

    // open file
    f_source = fopen(args.positionals[1], "rb");
    if (!f_source) {
        errorf("Failed to open %s.\n", args.positionals[1]);
        free(headers);
        return 1;
    }

    // create folder
    if (create_folders(args.positionals[2])) {
        errorf("Failed to create output folder %s.\n", args.positionals[2]);
        fclose(f_source);
        free(headers);
        return 2;
    }

    // create header extensions file
    strcpy(full_path, args.positionals[2]);
    strcat(full_path, PATHSEP_STR);
    strcat(full_path, "$PBOPREFIX$");
    if (access(full_path, F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", full_path);
        fclose(f_source);
        free(headers);
        return 3;
    }

    // read header extensions
    fseek(f_source, 1, SEEK_SET);
    fgets(buffer, 5, f_source);
    if (strncmp(buffer, "sreV", 4) == 0) {
        fseek(f_source, 21, SEEK_SET);

        // open header extensions file
        f_target = fopen(full_path, "wb");
        if (!f_target) {
            errorf("Failed to open file %s.\n", full_path);
            fclose(f_source);
            free(headers);
            return 4;
        }

        // read all header extensions
        i = 0;
        reading_headerext = false;
        while (true) {
            fp_tmp = ftell(f_source);
            buffer[i++] = fgetc(f_source);
            buffer[i] = '\0';
            if (buffer[i - 1] == '\0') {
                if (buffer[i - 2] == '\0') {
                    break;
                } else {
                    fputs(buffer, f_target);
                    if (reading_headerext) {
                        fputc('\n', f_target);
                        reading_headerext = false;
                    } else {
                        fputc('=', f_target);
                        reading_headerext = true;
                    }
                    i = 0;
                }
            }
        }
        fclose(f_target);
    } else {
        fseek(f_source, 0, SEEK_SET);
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
        fclose(f_source);
        free(headers);
        return 5;
    }

    // read files
    for (i = 0; i < num_files; i++) {
        // check for garbage
        if (is_garbage(&headers[i])) {
            fseek(f_source, headers[i].data_size, SEEK_CUR);
            continue;
        }

        // check if file is excluded
        for (j = 0; j < args.num_excludefiles; j++) {
            if (matches_glob(headers[i].name, args.excludefiles[j]))
                break;
        }
        if (j < args.num_excludefiles) {
            fseek(f_source, headers[i].data_size, SEEK_CUR);
            continue;
        }

        // check if file is included
        for (j = 1; j < args.num_includefolders; j++) {
            if (matches_glob(headers[i].name, args.includefolders[j]))
                break;
        }
        if (args.num_includefolders > 1 && j == args.num_includefolders) {
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
        strcpy(full_path, args.positionals[2]);
        strcat(full_path, PATHSEP_STR);
        strcat(full_path, headers[i].name);

        // create containing folder
        strcpy(buffer, full_path);
        if (strrchr(buffer, PATHSEP) != NULL) {
            *strrchr(buffer, PATHSEP) = 0;
            if (create_folders(buffer)) {
                errorf("Failed to create folder %s.\n", buffer);
                fclose(f_source);
                return 6;
            }
        }

        // open target file
        if (access(full_path, F_OK) != -1 && !args.force) {
            errorf("File %s already exists and --force was not set.\n", full_path);
            fclose(f_source);
            return 7;
        }
        f_target = fopen(full_path, "wb");
        if (!f_target) {
            errorf("Failed to open file %s.\n", full_path);
            fclose(f_source);
            return 8;
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
    free(headers);

    return 0;
}


int cmd_cat() {
    extern struct arguments args;
    extern char *current_target;
    FILE *f_source;
    int num_files;
    int file_index;
    long i;
    long j;
    long fp_tmp;
    char buffer[2048];
    struct header *headers;

    if (args.num_positionals < 3)
        return 128;

    headers = (struct header *)safe_malloc(sizeof(struct header) * MAXFILES);

    current_target = args.positionals[1];

    // open file
    f_source = fopen(args.positionals[1], "rb");
    if (!f_source) {
        errorf("Failed to open %s.\n", args.positionals[1]);
        free(headers);
        return 1;
    }

    // read header extensions
    fseek(f_source, 1, SEEK_SET);
    fgets(buffer, 5, f_source);
    if (strncmp(buffer, "sreV", 4) == 0) {
        fseek(f_source, 21, SEEK_SET);
        i = 0;
        while (true) {
            fp_tmp = ftell(f_source);
            buffer[i++] = fgetc(f_source);
            buffer[i] = '\0';
            if (buffer[i - 1] == '\0' && buffer[i - 2] == '\0')
                break;
        }
    } else {
        fseek(f_source, 0, SEEK_SET);
    }

    // read headers
    file_index = -1;
    for (num_files = 0; num_files <= MAXFILES; num_files++) {
        fp_tmp = ftell(f_source);
        fgets(buffer, sizeof(buffer), f_source);
        fseek(f_source, fp_tmp + strlen(buffer) + 1, SEEK_SET);
        if (strlen(buffer) == 0) {
            fseek(f_source, sizeof(uint32_t) * 5, SEEK_CUR);
            break;
        }

        if (stricmp(args.positionals[2], buffer) == 0)
            file_index = num_files;

        strcpy(headers[num_files].name, buffer);
        fread(&headers[num_files].packing_method, sizeof(uint32_t), 1, f_source);
        fread(&headers[num_files].original_size, sizeof(uint32_t), 1, f_source);
        fseek(f_source, sizeof(uint32_t) * 2, SEEK_CUR);
        fread(&headers[num_files].data_size, sizeof(uint32_t), 1, f_source);
    }

    if (num_files > MAXFILES) {
        errorf("Maximum number of files (%i) exceeded.\n", MAXFILES);
        fclose(f_source);
        free(headers);
        return 4;
    }

    if (file_index == -1) {
        errorf("PBO does not contain the file %s.\n", args.positionals[2]);
        fclose(f_source);
        free(headers);
        return 5;
    }

    // read files
    for (i = 0; i < num_files; i++) {
        if (i != file_index) {
            fseek(f_source, headers[i].data_size, SEEK_CUR);
            continue;
        }

        // write to file
        for (j = 0; headers[i].data_size - j >= sizeof(buffer); j += sizeof(buffer)) {
            fread(buffer, sizeof(buffer), 1, f_source);
            fwrite(buffer, sizeof(buffer), 1, stdout);
        }
        fread(buffer, headers[i].data_size - j, 1, f_source);
        fwrite(buffer, headers[i].data_size - j, 1, stdout);
    }

    // clean up
    fclose(f_source);
    free(headers);

    return 0;
}
