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
#include <unistd.h>

#include "sha1.h"
#include "docopt.h"
#include "filesystem.h"
#include "build.h"


int write_header_to_pbo(char *root, char *source, char *target) {
    FILE *f_source;
    FILE *f_target;
    char filename[1024];

    printf("Writing header: %s\n", source + strlen(root) + 1);

    filename[0] = 0;
    strcat(filename, source + strlen(root) + 1);

    f_target = fopen(target, "a");
    if (!f_target)
        return -1;

    fwrite(filename, strlen(filename), 1, f_target);

    // replace pathseps on linux
#ifndef _WIN32
    char tmp[1024];
    char *p = NULL;
    tmp[0] = 0;
    for (p = filename; *p; p++) {
        tmp[strlen(tmp) + 1] = 0;
        if (*p == '\\')
            tmp[strlen(tmp) - 1] = '/';
        else
            tmp[strlen(tmp) - 1] = *p;
    }
    filename[0] = 0;
    strcat(filename, tmp);
#endif

    struct {
        uint32_t method;
        uint32_t originalsize;
        uint32_t reserved;
        uint32_t timestamp;
        uint32_t datasize;
    } header;
    header.method = 0;
    header.originalsize = 0;
    header.reserved = 0;
    header.timestamp = 0;

    f_source = fopen(source, "r");
    if (!f_source)
        return -2;
    fseek(f_source, 0, SEEK_END);
    header.datasize = ftell(f_source);
    fclose(f_source);

    fwrite(&header, sizeof(header), 1, f_target);
    fclose(f_target);

    return 0;
}


int write_data_to_pbo(char *root, char *source, char *target) {
    FILE *f_source;
    FILE *f_target;
    char buffer[4096];
    int datasize;
    int i;

    printf("Writing file: %s\n", source + strlen(root) + 1);

    f_source = fopen(source, "r");
    if (!f_source)
        return -1;
    fseek(f_source, 0, SEEK_END);
    datasize = ftell(f_source);

    f_target = fopen(target, "a");
    if (!f_target)
        return -2;

    fseek(f_source, 0, SEEK_SET);
    for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
        fread(buffer, sizeof(buffer), 1, f_source);
        fwrite(buffer, sizeof(buffer), 1, f_target);
    }
    fread(buffer, datasize - i, 1, f_source);
    fwrite(buffer, datasize - i, 1, f_target);

    fclose(f_source);
    fclose(f_target);

    return 0;
}


int hash_file(char *path, unsigned char *hash) {
    SHA1Context sha;
    FILE *file;
    int filesize, i;
    char buffer[4096];

    SHA1Reset(&sha);

    file = fopen(path, "r");
    if (!file)
        return -1;

    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    for (i = 0; filesize - i >= sizeof(buffer); i += sizeof(buffer)) {
        fread(buffer, sizeof(buffer), 1, file);
        SHA1Input(&sha, (const unsigned char *)buffer, sizeof(buffer));
    }
    fread(buffer, filesize - i, 1, file);
    SHA1Input(&sha, (const unsigned char *)buffer, filesize - i);

    if (!SHA1Result(&sha))
        return -2;

    memcpy(hash, sha.Message_Digest, 20);

    return 0;
}


int build(DocoptArgs args) {
    // check if target already exists
    FILE *f_target;
    if (access(args.target, F_OK) != -1 && !args.force) {
        printf("File %s already exists and --force was not set.\n", args.target);
        return 1;
    }

    // remove trailing slash in source
    if (args.source[strlen(args.source) - 1] == PATHSEP)
        args.source[strlen(args.source) - 1] = 0;

    // get addon prefix
    char prefixpath[1024];
    char addonprefix[512];
    FILE *f_prefix;
    prefixpath[0] = 0;
    strcat(prefixpath, args.source);
#ifdef _WIN32
    strcat(prefixpath, "\\$PBOPREFIX$");
#else
    strcat(prefixpath, "/$PBOPREFIX$");
#endif
    f_prefix = fopen(prefixpath, "r");
    if (!f_prefix)
        strcat(addonprefix, "placeholder"); // @todo
    else
        fgets(addonprefix, sizeof(addonprefix), f_prefix);
    if (addonprefix[strlen(addonprefix) - 1] == '\n')
        addonprefix[strlen(addonprefix) - 1] = '\0';

    // write header extension
    printf("Writing header extension...\n");
    f_target = fopen(args.target, "w");
    fwrite("\0sreV\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0prefix\0", 28, 1, f_target);
    fwrite(addonprefix, strlen(addonprefix), 1, f_target);
    fwrite("\0\0", 2, 1, f_target);

    // replace pathseps on linux
#ifndef _WIN32
    char tmp[512];
    char *p = NULL;
    tmp[0] = 0;
    for (p = addonprefix; *p; p++) {
        if (*p == '\\' && tmp[strlen(tmp) - 1] == '/')
            continue;
        if (*p == '\\')
            tmp[strlen(tmp)] = '/';
        else
            tmp[strlen(tmp)] = *p;
        tmp[strlen(tmp) + 1] = 0;
    }
    addonprefix[0] = 0;
    strcat(addonprefix, tmp);
#endif

    // create and prepare temp folder
    printf("Creating temp folder ...\n\n");
    char tempfolder[1024];
    if (create_temp_folder(addonprefix, tempfolder, sizeof(tempfolder))) {
        printf("Failed to create temp folder.\n");
        return 2;
    }
    if (copy_directory(args.source, tempfolder)) {
        printf("Failed to copy to temp folder.\n");
        return 3;
    }

    // @todo: preprocess and binarize stuff if required

    // write headers to file
    if (traverse_directory(tempfolder, write_header_to_pbo, args.target)) {
        printf("Failed to write some file header(s) to PBO.\n");
        return 4;
    }

    // header boundary
    printf("Writing header boundary ...\n\n");
    f_target = fopen(args.target, "a");
    if (!f_target) {
        printf("Failed to write header boundary to PBO.\n");
        return 5;
    }
    for (int i; i < 21; i++)
        fputc(0, f_target);
    fclose(f_target);

    // write contents to file
    if (traverse_directory(tempfolder, write_data_to_pbo, args.target)) {
        printf("Failed to pack some file(s) into the PBO.\n");
        return 6;
    }

    // write checksum to file
    printf("\nWriting checksum ...\n");
    unsigned char checksum[21];
    checksum[0] = 0;
    printf("%s\n", checksum);
    hash_file(args.target, checksum+1);
    printf("%s\n", checksum);
    f_target = fopen(args.target, "a");
    if (!f_target) {
        printf("Failed to write checksum to file.\n");
        return 7;
    }
    fwrite(checksum, 21, 1, f_target);
    fclose(f_target);

    // remove temp folder
    printf("\nRemoving temp folder ...\n");
    if (remove_temp_folder()) {
        printf("Failed to remove temp folder.\n");
        return 7;
    }

    return 0;
}
