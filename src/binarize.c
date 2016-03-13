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


int mlod2odol(char *source, char *target) {
    return 0;
}


void trim_leading(char *string) {
    /*
     * Trims leading tabs and spaces on the string.
     */

    char tmp[4096];
    char *ptr = tmp;
    strncpy(tmp, string, 4096);
    while (*ptr == ' ' || *ptr == '\t')
        ptr++;
    strncpy(string, ptr, 4096 - (ptr - tmp));
}


bool matches_includepath(char *path, char *includepath, char *includefolder) {
    /*
     * Checks if a given file can be matched to an include path by traversing
     * backwards through the filesystem until a $PBOPREFIX$ file is found.
     * If the prefix file, together with the diretory strucure, matches the
     * included path, true is returned.
     */

    int i;
    char cwd[2048];
    char prefixpath[2048];
    char prefixedpath[2048];
    char *ptr;
    FILE *f_prefix;

    strncpy(cwd, path, 2048);
    ptr = cwd + strlen(cwd);

    while (strcmp(includefolder, cwd) != 0) {
        while (*ptr != PATHSEP)
            ptr--;
        *ptr = 0;

        strncpy(prefixpath, cwd, 2048);
        prefixpath[strlen(prefixpath) + 2] = 0;
        prefixpath[strlen(prefixpath)] = PATHSEP;
        strcat(prefixpath, "$PBOPREFIX$");

        f_prefix = fopen(prefixpath, "r");
        if (!f_prefix)
            continue;

        fgets(prefixedpath, sizeof(prefixedpath), f_prefix);
        if (prefixedpath[strlen(prefixedpath) - 1] == '\n')
            prefixedpath[strlen(prefixedpath) - 1] = 0;
        if (prefixedpath[strlen(prefixedpath) - 1] == '\\')
            prefixedpath[strlen(prefixedpath) - 1] = 0;

        strcat(prefixedpath, path + strlen(cwd));

        for (i = 0; i < strlen(prefixedpath); i++) {
            if (prefixedpath[i] == '/')
                prefixedpath[i] = '\\';
        }

        // compensate for missing leading slash in PBOPREFIX
        if (prefixedpath[0] != '\\')
            return (strcmp(prefixedpath, includepath+1) == 0);
        else
            return (strcmp(prefixedpath, includepath) == 0);
    }

    return false;
}


int find_file(char *includepath, char *origin, char *includefolder, char *actualpath, char *cwd) {
    /*
     * Finds the file referenced in includepath in the includefolder. origin
     * describes the file in which the include is used (used for relative
     * includes). actualpath holds the return pointer. The 4th arg is used for
     * recursion on Windows and should be passed as NULL initially.
     *
     * Returns 0 on success, 1 on error and 2 if no file could be found.
     *
     * Please note that relative includes always return a path, even if that
     * file exists.
     */

    // relative include, this shit is easy
    if (includepath[0] != '\\') {
        strncpy(actualpath, origin, 2048);
        char *target = actualpath + strlen(actualpath) - 1;
        while (*target != PATHSEP)
            target--;
        strncpy(target + 1, includepath, 2046 - (target - actualpath));

#ifndef _WIN32
        for (int i; i < strlen(actualpath); i++) {
            if (actualpath[i] == '\\')
                actualpath[i] = '/';
        }
#endif

        return 0;
    }

    char filename[2048];
    char *ptr = includepath + strlen(includepath);

    while (*ptr != '\\')
        ptr--;
    ptr++;

    strncpy(filename, ptr, 2048);

#ifdef _WIN32
    if (cwd == NULL)
        return find_file(includepath, origin, includefolder, actualpath, includefolder);

    WIN32_FIND_DATA file;
    HANDLE handle = NULL;
    char mask[2048];
    int success;

    GetFullPathName(includefolder, 2048, mask, NULL);
    sprintf(mask, "%s\\*", mask);

    handle = FindFirstFile(mask, &file);
    if (handle == INVALID_HANDLE_VALUE)
        return 1;

    do {
        sprintf(mask, "%s\\%s", cwd, file.cFileName);
        if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            success = find_file(includepath, origin, includefolder, actualpath, mask);
            if (success)
                return success;
        } else {
            if (strcmp(filename, file.cFileName) == 0 && matches_includepath(mask, includepath, includefolder)) {
                strncpy(actualpath, mask, 2048);
                return 0;
            }
        }
    } while (FindNextFile(handle, &file));

    FindClose(handle);
#else
    FTS *tree;
    FTSENT *f;
    char *argv[] = { includefolder, NULL };

    tree = fts_open(argv, FTS_LOGICAL | FTS_NOSTAT, NULL);
    if (tree == NULL)
        return 1;

    while ((f = fts_read(tree))) {
        switch (f->fts_info) {
            case FTS_DNR: return 2;
            case FTS_ERR: return 3;
            case FTS_NS: continue;
            case FTS_DP: continue;
            case FTS_D: continue;
            case FTS_DC: continue;
        }

        if (strcmp(filename, f->fts_name) == 0 && matches_includepath(f->fts_path, includepath, includefolder)) {
            strncpy(actualpath, f->fts_path, 2048);
            return 0;
        }
    }

#endif

    return 2;
}


int resolve_includes(char *source, FILE *f_target, char *includefolder, char definitions[512][256]) {
    /*
     * Writes the contents of source into the target file pointer, while
     * recursively resolving includes using the includefolder for finding
     * included files.
     *
     * Returns 0 on success, a positive integer on failure.
     *
     * Since includes might be found inside #ifs, there needs to be some basic
     * preprocessing of defines as well. Things that are not supported here:
     *     - constants in/as include paths
     *     - nested ifs
     *     - undefining constants
     */

    int line = 0;
    int i = 0;
    int level = 0;
    int level_true = 0;
    int level_comment = 0;
    int success;
    size_t buffsize = 4096;
    char *buffer;
    char *ptr;
    char definition[256];
    char includepath[2048];
    char actualpath[2048];
    FILE *f_source;

    f_source = fopen(source, "r");
    if (!f_source) {
        printf("Failed to open %s.\n", source);
        return 1;
    }

    while (true) {
        line++;
        buffer = (char *)malloc(buffsize+1);
        if (getline(&buffer, &buffsize, f_source) == -1)
            break;

        trim_leading(buffer);

        if (level > level_true) {
            if ((strlen(buffer) < 5 || strncmp(buffer, "#else", 5) != 0) &&
                    (strlen(buffer) < 6 || strncmp(buffer, "#endif", 6) != 0))
                continue;
        }

        // Get the constant name
        if ((strlen(buffer) >= 9 && strncmp(buffer, "#define", 7) == 0) ||
                (strlen(buffer) >= 10 && (strncmp(buffer, "#ifdef", 6) == 0 ||
                strncmp(buffer, "#ifndef", 7) == 0))) {
            definition[0] = 0;
            ptr = buffer + 7;
            while (*ptr == ' ')
                ptr++;
            strncpy(definition, ptr, 256);
            ptr = definition;
            while (*ptr != ' ' && *ptr != '(' && *ptr != '\n')
                ptr++;
            *ptr = 0;

            if (strlen(definition) == 0) {
                printf("Missing definition in line %i of %s.\n", line, source);
                return 2;
            }
        }

        // Check for preprocessor commands
        if (level_comment == 0 && strlen(buffer) >= 9 && strncmp(buffer, "#define", 7) == 0) {
            i = 0;
            while (definitions[i] != NULL &&
                    strcmp(definitions[i], definition) != 0 && i <= 512)
                i++;

            if (i == 512) {
                printf("Maximum number of 512 definitions exceeded in line %i of %s.\n", line, source);
                return 3;
            }

            strncpy(definitions[i], definition, 256);
        } else if (level_comment == 0 && strlen(buffer) >= 10 &&
                (strncmp(buffer, "#ifdef", 6) == 0 || strncmp(buffer, "#ifndef", 7) == 0)) {
            level++;
            for (i = 0; i < 512; i++) {
                if (strcmp(definition, definitions[i]) == 0 &&
                        strncmp(buffer, "#ifndef", 7) == 0) {
                    level_true++;
                    continue;
                }
            }
        } else if (level_comment == 0 && strlen(buffer) >= 5 && strncmp(buffer, "#else", 5) == 0) {
            if (level == level_true)
                level_true--;
            else
                level_true = level;
        } else if (level_comment == 0 && strlen(buffer) >= 6 && strncmp(buffer, "#endif", 6) == 0) {
            if (level == 0) {
                printf("Unexpected #endif in line %i of %s.\n", line, source);
                return 4;
            }
            if (level == level_true)
                level_true--;
            level--;
        } else if (level_comment == 0 && strlen(buffer) >= 11 && strncmp(buffer, "#include", 8) == 0) {
            if (strchr(buffer, '"') == NULL) {
                printf("Failed to parse #include in line %i in %s.\n", line, source);
                return 5;
            }
            strncpy(includepath, strchr(buffer, '"') + 1, sizeof(includepath));
            if (strchr(includepath, '"') == NULL) {
                printf("Failed to parse #include in line %i in %s.\n", line, source);
                return 6;
            }
            *strchr(includepath, '"') = 0;
            if (find_file(includepath, source, includefolder, actualpath, NULL)) {
                printf("Failed to find %s in %s.\n", includepath, includefolder);
                return 7;
            }
            free(buffer);
            success = resolve_includes(actualpath, f_target, includefolder, definitions);
            if (success)
                return success;
            continue;
        }

        // Check for comment delimiters
        for (i = 0; i < strlen(buffer) - 1; i++) {
            if (buffer[i] == '/' && buffer[i+1] == '*')
                level_comment++;
            else if (buffer[i] == '*' && buffer[i+1] == '/')
                level_comment--;
            if (level_comment < 0)
                level_comment = 0;
        }

        printf("%s", buffer);
        fputs(buffer, f_target);
        free(buffer);
    }

    fclose(f_source);

    return 0;
}


int rapify_file(char *source, char *target, char *includefolder) {
    /*
     * Resolves macros/includes and rapifies the given file. If source and
     * target are identical, the target is overwritten.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    FILE *f_temp = tmpfile();

    // Write original file to temp and resolve includes
    char definitions[512][256] = {};
    int success = resolve_includes(source, f_temp, includefolder, definitions);
    if (success)
        return success;

    // Process definitions

    // Rapify file

    fclose(f_temp);

    return 0;
}


int binarize_file(char *source, char *target, char *includefolder) {
    /*
     * Binarize the given file. If source and target are identical, the target
     * is overwritten. If the source is a P3D, it is converted to ODOL. If the
     * source is a rapifiable type (cpp, rvmat, etc.), it is rapified.
     *
     * If the file type is not recognized, -1 is returned. 0 is returned on
     * success and a positive integer on error.
     *
     * The include folder argument is the path to be searched for file
     * inclusions.
     */

    // Get file type
    char fileext[64];
    strncpy(fileext, strrchr(source, '.'), 64);

    if (!strcmp(fileext, ".cpp") ||
            !strcmp(fileext, ".rvmat") ||
            !strcmp(fileext, ".ext"))
        return rapify_file(source, target, includefolder);
    if (!strcmp(fileext, ".p3d"))
        return mlod2odol(source, target);

    return -1;
}


int binarize(DocoptArgs args) {
    // check if target already exists
    if (access(args.target, F_OK) != -1 && !args.force) {
        printf("File %s already exists and --force was not set.\n", args.target);
        return 1;
    }

    // remove trailing slash in source and target
    if (args.source[strlen(args.source) - 1] == PATHSEP)
        args.source[strlen(args.source) - 1] = 0;
    if (args.target[strlen(args.target) - 1] == PATHSEP)
        args.target[strlen(args.target) - 1] = 0;

    char *includefolder = ".";
    if (args.include)
        includefolder = args.includefolder;

    if (includefolder[strlen(includefolder) - 1] == PATHSEP)
        includefolder[strlen(includefolder) - 1] = 0;

    int success = binarize_file(args.source, args.target, includefolder);

    if (success == -1) {
        printf("File is no P3D and doesn't seem rapifiable.\n");
        return 1;
    }

    return success;
}
