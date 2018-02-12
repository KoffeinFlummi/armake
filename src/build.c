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
#endif

#include "sha1.h"
#include "args.h"
#include "binarize.h"
#include "filesystem.h"
#include "utils.h"
#include "sign.h"
#include "build.h"


int binarize_callback(char *root, char *source, char *junk) {
    int success;
    char target[2048];

    strncpy(target, source, sizeof(target));

    if (strlen(target) > 10 &&
            strcmp(target + strlen(target) - 10, "config.cpp") == 0) {
        strcpy(target + strlen(target) - 3, "bin");
    }

    success = binarize(source, target);

    if (success > 0)
        return success * -1;

    return 0;
}


bool file_allowed(char *filename) {
    int i;
    extern struct arguments args;

    if (strcmp(filename, "$PBOPREFIX$") == 0)
        return false;

    for (i = 0; i < args.num_excludefiles; i++) {
        if (matches_glob(filename, args.excludefiles[i]))
            return false;
    }

    return true;
}


int write_header_to_pbo(char *root, char *source, char *target) {
    FILE *f_source;
    FILE *f_target;
    char filename[1024];

    filename[0] = 0;
    strcat(filename, source + strlen(root) + 1);

    if (!file_allowed(filename))
        return 0;

    f_target = fopen(target, "ab");
    if (!f_target)
        return -1;

    struct {
        uint32_t method;
        uint32_t originalsize;
        uint32_t reserved;
        uint32_t timestamp;
        uint32_t datasize;
    } header;
    header.method = 0;
    header.reserved = 0;
    header.timestamp = 0;

    f_source = fopen(source, "rb");
    if (!f_source) {
        fclose(f_target);
        return -2;
    }

    fseek(f_source, 0, SEEK_END);
    header.datasize = ftell(f_source);
    header.originalsize = header.datasize;
    fclose(f_source);

    // replace pathseps on linux
#ifndef _WIN32
    int i;
    for (i = 0; i < strlen(filename); i++) {
        if (filename[i] == '/')
            filename[i] = '\\';
    }
#endif

    fwrite(filename, strlen(filename), 1, f_target);
    fputc(0, f_target);

    fwrite(&header, sizeof(header), 1, f_target);
    fclose(f_target);

    return 0;
}


int write_data_to_pbo(char *root, char *source, char *target) {
    FILE *f_source;
    FILE *f_target;
    char buffer[4096];
    char filename[1024];
    int datasize;
    int i;

    filename[0] = 0;
    strcat(filename, source + strlen(root) + 1);

    if (!file_allowed(filename))
        return 0;

    f_source = fopen(source, "rb");
    if (!f_source)
        return -1;
    fseek(f_source, 0, SEEK_END);
    datasize = ftell(f_source);

    f_target = fopen(target, "ab");
    if (!f_target) {
        fclose(f_source);
        return -2;
    }

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
    unsigned temp;
    char buffer[4096];

    SHA1Reset(&sha);

    file = fopen(path, "rb");
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

    fclose(file);

    if (!SHA1Result(&sha))
        return -2;

    for (i = 0; i < 5; i++) {
        temp = sha.Message_Digest[i];
        sha.Message_Digest[i] = ((temp>>24)&0xff) |
            ((temp<<8)&0xff0000) | ((temp>>8)&0xff00) | ((temp<<24)&0xff000000);
    }

    memcpy(hash, sha.Message_Digest, 20);

    return 0;
}


int cmd_build() {
    extern char *current_target;
    int i;
    int j;
    int k;
    char buffer[512];
    bool valid;

    if (args.num_positionals != 3)
        return 128;

    current_target = args.positionals[1];

    // check if target already exists
    FILE *f_target;
    if (access(args.positionals[2], F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", args.positionals[2]);
        return 1;
    }

    // remove trailing slash in source
    if (args.positionals[1][strlen(args.positionals[1]) - 1] == '\\')
        args.positionals[1][strlen(args.positionals[1]) - 1] = 0;
    if (args.positionals[1][strlen(args.positionals[1]) - 1] == '/')
        args.positionals[1][strlen(args.positionals[1]) - 1] = 0;

    f_target = fopen(args.positionals[2], "wb");
    if (!f_target) {
        errorf("Failed to open %s.\n", args.positionals[2]);
        return 2;
    }
    fclose(f_target);

    // get addon prefix
    char prefixpath[1024];
    char addonprefix[512];
    FILE *f_prefix;
    prefixpath[0] = 0;
    strcat(prefixpath, args.positionals[1]);
    strcat(prefixpath, PATHSEP_STR);
    strcat(prefixpath, "$PBOPREFIX$");

    for (i = 0; i < args.num_headerextensions && args.headerextensions[i][0] != 0; i++) {
        k = 0;
        valid = false;
        for (j = 0; j <= strlen(args.headerextensions[i]); j++) {
            if (args.headerextensions[i][j] == '=' || args.headerextensions[i][j] == '\0') {
                if (strcmp(buffer, "prefix") == 0) {
                    k = 0;
                    valid = true;
                } else if (valid) {
                    strcat(addonprefix, buffer);
                } else {
                    break;
                }
            } else {
                buffer[k++] = args.headerextensions[i][j];
                buffer[k] = '\0';
            }
        }
    }

    if (!valid) {
        f_prefix = fopen(prefixpath, "rb");
        if (!f_prefix) {
            if (strrchr(args.positionals[1], PATHSEP) == NULL)
                strncpy(addonprefix, args.positionals[1], sizeof(addonprefix));
            else
                strncpy(addonprefix, strrchr(args.positionals[1], PATHSEP) + 1, sizeof(addonprefix));
        } else {
            fgets(addonprefix, sizeof(addonprefix), f_prefix);
            fclose(f_prefix);
        }

        if (addonprefix[strlen(addonprefix) - 1] == '\n')
            addonprefix[strlen(addonprefix) - 1] = '\0';
        if (addonprefix[strlen(addonprefix) - 1] == '\r')
            addonprefix[strlen(addonprefix) - 1] = '\0';
    }

    // replace pathseps on linux
#ifndef _WIN32
    char tmp[512] = "";
    char *p = NULL;
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
    char tempfolder[1024];
    if (create_temp_folder(addonprefix, tempfolder, sizeof(tempfolder))) {
        errorf("Failed to create temp folder.\n");
        remove_file(args.positionals[2]);
        return 2;
    }
    if (copy_directory(args.positionals[1], tempfolder)) {
        errorf("Failed to copy to temp folder.\n");
        remove_file(args.positionals[2]);
        remove_folder(tempfolder);
        return 3;
    }

    // preprocess and binarize stuff if required
    char nobinpath[1024];
    char notestpath[1024];
    strcpy(nobinpath, prefixpath);
    strcpy(notestpath, prefixpath);
    strcpy(nobinpath + strlen(nobinpath) - 11, "$NOBIN$");
    strcpy(notestpath + strlen(notestpath) - 11, "$NOBIN-NOTEST$");
    if (!args.packonly && access(nobinpath, F_OK) == -1 && access(notestpath, F_OK) == -1) {
        if (traverse_directory(tempfolder, binarize_callback, "")) {
            current_target = args.positionals[1];
            errorf("Failed to binarize some files.\n");
            remove_file(args.positionals[2]);
            remove_folder(tempfolder);
            return 4;
        }

        char configpath[2048];
        strcpy(configpath, tempfolder);
        strcat(configpath, "?config.cpp");
        configpath[strlen(tempfolder)] = PATHSEP;

        if (access(configpath, F_OK) != -1) {
#ifdef _WIN32
            if (!DeleteFile(configpath)) {
#else
            if (remove(configpath)) {
#endif
                remove_file(args.positionals[2]);
                remove_folder(tempfolder);
                return 5;
            }
        }
    }

    current_target = args.positionals[1];

    // write header extensions
    f_target = fopen(args.positionals[2], "wb");
    fwrite("\0sreV\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0prefix\0", 28, 1, f_target);
    // write addonprefix with windows pathseps
    for (i = 0; i <= strlen(addonprefix); i++) {
        if (addonprefix[i] == PATHSEP)
            fputc('\\', f_target);
        else
            fputc(addonprefix[i], f_target);
    }
    // write extra header extensions
    for (i = 0; i < args.num_headerextensions && args.headerextensions[i][0] != 0; i++) {
        k = 0;
        valid = false;
        for (j = 0; j <= strlen(args.headerextensions[i]); j++) {
            if (args.headerextensions[i][j] == '=' || args.headerextensions[i][j] == '\0') {
                // prefix is already written above
                if (strcmp(buffer, "prefix") == 0)
                    break;

                // validate
                if (args.headerextensions[i][j] == '\0' && !valid) {
                    errorf("Invalid header extension format (%s).\n", args.headerextensions[i]);
                    remove_file(args.positionals[2]);
                    remove_folder(tempfolder);
                    return 6;
                }

                // write
                fputs(buffer, f_target);
                fputc(0, f_target);
                k = 0;
                valid = true;
            } else {
                buffer[k++] = args.headerextensions[i][j];
                buffer[k] = '\0';
            }
        }
    }
    fputc(0, f_target);
    fclose(f_target);

    // write headers to file
    if (traverse_directory(tempfolder, write_header_to_pbo, args.positionals[2])) {
        errorf("Failed to write some file header(s) to PBO.\n");
        remove_file(args.positionals[2]);
        remove_folder(tempfolder);
        return 7;
    }

    // header boundary
    f_target = fopen(args.positionals[2], "ab");
    if (!f_target) {
        errorf("Failed to write header boundary to PBO.\n");
        remove_file(args.positionals[2]);
        remove_folder(tempfolder);
        return 8;
    }
    for (i = 0; i < 21; i++)
        fputc(0, f_target);
    fclose(f_target);

    // write contents to file
    if (traverse_directory(tempfolder, write_data_to_pbo, args.positionals[2])) {
        errorf("Failed to pack some file(s) into the PBO.\n");
        remove_file(args.positionals[2]);
        remove_folder(tempfolder);
        return 9;
    }

    // write checksum to file
    unsigned char checksum[20];
    hash_file(args.positionals[2], checksum);
    f_target = fopen(args.positionals[2], "ab");
    if (!f_target) {
        errorf("Failed to write checksum to file.\n");
        remove_file(args.positionals[2]);
        remove_folder(tempfolder);
        return 10;
    }
    fputc(0, f_target);
    fwrite(checksum, 20, 1, f_target);
    fclose(f_target);

    // remove temp folder
    if (remove_folder(tempfolder)) {
        errorf("Failed to remove temp folder.\n");
        return 11;
    }

    // sign pbo
    if (args.privatekey) {
        char keyname[512];
        char path_signature[2048];

        if (strcmp(strrchr(args.privatekey, '.'), ".biprivatekey") != 0) {
            errorf("File %s doesn't seem to be a valid private key.\n", args.positionals[1]);
            return 1;
        }

        if (strchr(args.privatekey, PATHSEP) == NULL)
            strcpy(keyname, args.privatekey);
        else
            strcpy(keyname, strrchr(args.privatekey, PATHSEP) + 1);
        *strrchr(keyname, '.') = 0;

        strcpy(path_signature, args.positionals[2]);
        strcat(path_signature, ".");
        strcat(path_signature, keyname);
        strcat(path_signature, ".bisign");

        // check if target already exists
        if (access(path_signature, F_OK) != -1 && !args.force) {
            errorf("File %s already exists and --force was not set.\n", path_signature);
            return 2;
        }

        if (sign_pbo(args.positionals[2], args.privatekey, path_signature)) {
            errorf("Failed to sign file.\n");
            return 3;
        }
    }

    return 0;
}
