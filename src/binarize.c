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

#include "args.h"
#include "filesystem.h"
#include "utils.h"
#include "rapify.h"
#include "p3d.h"
#include "binarize.h"
#include "utils.h"


bool warned_bi_not_found = false;


#ifdef _WIN32
bool file_exists(char *path) {
    unsigned long attrs = GetFileAttributes(path);
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}


char *find_root(char *source) {
    char *root = (char *)malloc(2048);
    char *candidate = (char *)malloc(2048);

    GetFullPathName(source, 2048, root, NULL);

    while (true) {
        if (strrchr(root, '\\') == NULL) {
            strcpy(candidate, "config.cpp");
            if (access(candidate, F_OK) != -1) {
                free(candidate);
                free(root);
                return ".\\";
            }
            return NULL;
        }

        *(strrchr(root, '\\') + 1) = 0;

        strcpy(candidate, root);
        strcat(candidate, "config.cpp");
        if (access(candidate, F_OK) != -1) {
            free(candidate);
            return root;
        }

        *(strrchr(root, '\\')) = 0;
    }
}


int attempt_bis_binarize(char *source, char *target) {
    /*
     * Attempts to find and use the BI binarize.exe for binarization. If the
     * exe is not found, a negative integer is returned. 0 is returned on
     * success and a positive integer on failure.
     */

    extern char *current_target;
    SECURITY_ATTRIBUTES secattr = { sizeof(secattr) };
    STARTUPINFO info = { sizeof(info) };
    PROCESS_INFORMATION processInfo;
    long unsigned buffsize;
    long success;
    int32_t num_lods;
    int i;
    int j;
    int k;
    bool is_rtm;
    char command[2048];
    char temp[2048];
    char tempfolder[2048];
    char filename[2048];
    char *dependencies[MAXTEXTURES];
    char *root;
    FILE *f_source;
    struct mlod_lod *mlod_lods;

    current_target = source;

    if (getenv("NATIVEBIN"))
        return -1;

    is_rtm = !strcmp(source + strlen(source) - 4, ".rtm");

    for (i = 0; i < strlen(source); i++)
        source[i] = (source[i] == '/') ? '\\' : source[i];

    for (i = 0; i < strlen(target); i++)
        target[i] = (target[i] == '/') ? '\\' : target[i];

    // Find binarize.exe
    buffsize = sizeof(command);
    success = RegGetValue(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath",
            RRF_RT_ANY, NULL, command, &buffsize);

    if (success != ERROR_SUCCESS)
        return -2;

    strcat(command, "/steamapps/common/Arma 3 Tools/Binarize/binarize.exe");

    if (!file_exists(command))
        return -3;

    // Read P3D and create a list of required files
    if (!is_rtm) {
        f_source = fopen(source, "rb");
        if (!f_source)
            return 1;

        fseek(f_source, 8, SEEK_SET);
        fread(&num_lods, 4, 1, f_source);
        mlod_lods = (struct mlod_lod *)safe_malloc(sizeof(struct mlod_lod) * num_lods);
        num_lods = read_lods(f_source, mlod_lods, num_lods);
        fflush(stdout);
        if (num_lods < 0)
            return 2;

        fclose(f_source);

        memset(dependencies, 0, sizeof(dependencies));
        for (i = 0; i < num_lods; i++) {
            for (j = 0; j < mlod_lods[i].num_faces; j++) {
                if (strlen(mlod_lods[i].faces[j].texture_name) > 0 && mlod_lods[i].faces[j].texture_name[0] != '#') {
                    for (k = 0; k < MAXTEXTURES; k++) {
                        if (dependencies[k] == 0)
                            break;
                        if (stricmp(mlod_lods[i].faces[j].texture_name, dependencies[k]) == 0)
                            break;
                    }
                    if (k < MAXTEXTURES && dependencies[k] == 0) {
                        dependencies[k] = (char *)safe_malloc(2048);
                        strcpy(dependencies[k], mlod_lods[i].faces[j].texture_name);
                    }
                }
                if (strlen(mlod_lods[i].faces[j].material_name) > 0 && mlod_lods[i].faces[j].material_name[0] != '#') {
                    for (k = 0; k < MAXTEXTURES; k++) {
                        if (dependencies[k] == 0)
                            break;
                        if (stricmp(mlod_lods[i].faces[j].material_name, dependencies[k]) == 0)
                            break;
                    }
                    if (k < MAXTEXTURES && dependencies[k] == 0) {
                        dependencies[k] = (char *)safe_malloc(2048);
                        strcpy(dependencies[k], mlod_lods[i].faces[j].material_name);
                    }
                }
            }

            free(mlod_lods[i].points);
            free(mlod_lods[i].facenormals);
            free(mlod_lods[i].faces);
            free(mlod_lods[i].mass);
            free(mlod_lods[i].sharp_edges);

            for (j = 0; j < mlod_lods[i].num_selections; j++) {
                free(mlod_lods[i].selections[j].points);
                free(mlod_lods[i].selections[j].faces);
            }

            free(mlod_lods[i].selections);
        }
        free(mlod_lods);
    }

    // Create a temporary folder to isolate the target file and copy it there
    if (strchr(source, '\\') != NULL)
        strcpy(filename, strrchr(source, '\\') + 1);
    else
        strcpy(filename, source);

    if (create_temp_folder(filename, tempfolder, sizeof(tempfolder))) {
        errorf("Failed to create temp folder.\n");
        return 1;
    }

    strcpy(temp, (strchr(target, PATHSEP) == NULL) ? target : strrchr(target, PATHSEP) + 1);
    strcpy(filename, tempfolder);
    strcat(filename, temp);

    GetFullPathName(source, 2048, temp, NULL);

    if (copy_file(temp, filename)) {
        errorf("Failed to copy %s to temp folder.\n", temp);
        return 2;
    }

    // Try to find the required files and copy them there too
    root = find_root(source);

    strcpy(temp, root);
    strcat(temp, "config.cpp");
    strcpy(filename, tempfolder);
    strcat(filename, "config.cpp");
    copy_file(temp, filename);

    strcpy(temp, root);
    strcat(temp, "model.cfg");
    strcpy(filename, tempfolder);
    strcat(filename, "model.cfg");
    copy_file(temp, filename);

    free(root);

    if (!is_rtm) {
        for (i = 0; i < MAXTEXTURES; i++) {
            if (dependencies[i] == 0)
                break;

            *filename = 0;
            if (dependencies[i][0] != '\\')
                strcpy(filename, "\\");
            strcat(filename, dependencies[i]);

            if (find_file(filename, "", temp)) {
                lwarningf(source, -1, "Failed to find file %s.\n", filename);
                free(dependencies[i]);
                continue;
            }

            strcpy(filename, tempfolder);
            strcat(filename, dependencies[i]);

            if (copy_file(temp, filename)) {
                errorf("Failed to copy %s to temp folder.\n", temp);
                return 3;
            }

            free(dependencies[i]);
        }
    }

    // Call binarize.exe
    strcpy(temp, command);
    sprintf(command, "\"%s\"", temp);

    strcat(command, " -norecurse -always -silent -maxProcesses=0 ");
    strcat(command, tempfolder);
    strcat(command, " ");
    GetFullPathName(target, 2048, temp, NULL);
    *(strrchr(temp, PATHSEP)) = 0;
    strcat(command, temp);

    if (getenv("BIOUTPUT"))
        debugf("cmdline: %s\n", command);

    if (!getenv("BIOUTPUT")) {
        secattr.lpSecurityDescriptor = NULL;
        secattr.bInheritHandle = TRUE;
        info.hStdOutput = info.hStdError = CreateFile("NUL", GENERIC_WRITE, 0, &secattr, OPEN_EXISTING, 0, NULL);
        info.dwFlags |= STARTF_USESTDHANDLES;
    }

    if (CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)) {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    } else {
        errorf("Failed to binarize %s.\n", source);
        return 3;
    }

    if (getenv("BIOUTPUT"))
        debugf("done with binarize.exe\n");

    // Clean Up
    if (remove_folder(tempfolder)) {
        errorf("Failed to remove temp folder.\n");
        return 4;
    }

    return success;
}
#endif


int binarize(char *source, char *target) {
    /*
     * Binarize the given file. If source and target are identical, the target
     * is overwritten. If the source is a P3D, it is converted to ODOL. If the
     * source is a rapifiable type (cpp, ext, etc.), it is rapified.
     *
     * If the file type is not recognized, -1 is returned. 0 is returned on
     * success and a positive integer on error.
     */

    char fileext[64];
#ifdef _WIN32
    int success;
    extern bool warned_bi_not_found;
#endif

    if (strchr(source, '.') == NULL)
        return -1;

    strncpy(fileext, strrchr(source, '.'), 64);

    if (!strcmp(fileext, ".cpp") ||
            !strcmp(fileext, ".rvmat") ||
            !strcmp(fileext, ".ext"))
        return rapify_file(source, target);

    if (!strcmp(fileext, ".p3d") ||
            !strcmp(fileext, ".rtm")) {
#ifdef _WIN32
        success = attempt_bis_binarize(source, target);
        if (success >= 0)
            return success;
        if (!warned_bi_not_found) {
            lwarningf(source, -1, "Failed to find BI tools, using internal binarizer.\n");
            warned_bi_not_found = true;
        }
#endif
        if (!strcmp(fileext, ".p3d"))
            return mlod2odol(source, target);
    }

    return -1;
}


int cmd_binarize() {
    int success;

    if (args.num_positionals == 1) {
        return 128;
    } else if (args.num_positionals == 2) {
        success = binarize(args.positionals[1], "-");
    } else {
        // check if target already exists
        if (access(args.positionals[2], F_OK) != -1 && !args.force) {
            errorf("File %s already exists and --force was not set.\n", args.positionals[2]);
            return 1;
        }

        success = binarize(args.positionals[1], args.positionals[2]);
    }

    if (success == -1) {
        errorf("File is no P3D and doesn't seem rapifiable.\n");
        return 1;
    }

    return success;
}
