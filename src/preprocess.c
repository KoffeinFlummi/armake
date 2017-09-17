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
#include <errno.h>

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
#include "preprocess.h"


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
        strcat(prefixpath, PATHSEP_STR);
        strcat(prefixpath, "$PBOPREFIX$");

        f_prefix = fopen(prefixpath, "rb");
        if (!f_prefix)
            continue;

        fgets(prefixedpath, sizeof(prefixedpath), f_prefix);
        fclose(f_prefix);

        if (prefixedpath[strlen(prefixedpath) - 1] == '\n')
            prefixedpath[strlen(prefixedpath) - 1] = 0;
        if (prefixedpath[strlen(prefixedpath) - 1] == '\r')
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


int find_file_helper(char *includepath, char *origin, char *includefolder, char *actualpath, char *cwd) {
    /*
     * Finds the file referenced in includepath in the includefolder. origin
     * describes the file in which the include is used (used for relative
     * includes). actualpath holds the return pointer. The 4th arg is used for
     * recursion on Windows and should be passed as NULL initially.
     *
     * Returns 0 on success, 1 on error and 2 if no file could be found.
     *
     * Please note that relative includes always return a path, even if that
     * file does not exist.
     */

    // relative include, this shit is easy
    if (includepath[0] != '\\') {
        strncpy(actualpath, origin, 2048);
        char *target = actualpath + strlen(actualpath) - 1;
        while (*target != PATHSEP && target >= actualpath)
            target--;
        strncpy(target + 1, includepath, 2046 - (target - actualpath));

#ifndef _WIN32
        int i;
        for (i = 0; i < strlen(actualpath); i++) {
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
        return find_file_helper(includepath, origin, includefolder, actualpath, includefolder);

    WIN32_FIND_DATA file;
    HANDLE handle = NULL;
    char mask[2048];

    GetFullPathName(includefolder, 2048, includefolder, NULL);

    GetFullPathName(cwd, 2048, mask, NULL);
    sprintf(mask, "%s\\*", mask);

    handle = FindFirstFile(mask, &file);
    if (handle == INVALID_HANDLE_VALUE)
        return 1;

    do {
        if (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0)
            continue;

        GetFullPathName(cwd, 2048, mask, NULL);
        sprintf(mask, "%s\\%s", mask, file.cFileName);
        if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!find_file_helper(includepath, origin, includefolder, actualpath, mask))
                return 0;
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
            case FTS_DNR:
            case FTS_ERR:
                fts_close(tree);
                return 2;
            case FTS_NS: continue;
            case FTS_DP: continue;
            case FTS_D: continue;
            case FTS_DC: continue;
        }

        if (strcmp(filename, f->fts_name) == 0 && matches_includepath(f->fts_path, includepath, includefolder)) {
            strncpy(actualpath, f->fts_path, 2048);
            fts_close(tree);
            return 0;
        }
    }

    fts_close(tree);
#endif

    // check for file without pboprefix
    strncpy(filename, includefolder, sizeof(filename));
    strncat(filename, includepath, sizeof(filename) - strlen(filename) - 1);
#ifndef _WIN32
    int i;
    for (i = 0; i < strlen(filename); i++) {
        if (filename[i] == '\\')
            filename[i] = '/';
    }
#endif
    if (access(filename, F_OK) != -1) {
        strcpy(actualpath, filename);
        return 0;
    }

    return 2;
}


int find_file(char *includepath, char *origin, char *actualpath) {
    /*
     * Finds the file referenced in includepath in the includefolder. origin
     * describes the file in which the include is used (used for relative
     * includes). actualpath holds the return pointer. The 4th arg is used for
     * recursion on Windows and should be passed as NULL initially.
     *
     * Returns 0 on success, 1 on error and 2 if no file could be found.
     *
     * Please note that relative includes always return a path, even if that
     * file does not exist.
     */

    int i;
    int success;
    extern char include_folders[MAXINCLUDEFOLDERS][512];

    for (i = 0; i < MAXINCLUDEFOLDERS && include_folders[i][0] != 0; i++) {
        success = find_file_helper(includepath, origin, include_folders[i], actualpath, NULL);

        if (success != 2)
            return success;
    }

    return 2;
}


int preprocess_prepare(char *source, FILE *f_target, struct lineref *lineref) {
    /*
     * Writes the contents of source into the target file pointer, while
     * recursively resolving constants and includes using the includefolder
     * for finding included files.
     *
     * Returns 0 on success, a positive integer on failure.
     */

    extern int current_operation;
    extern char current_target[2048];
    extern char include_stack[MAXINCLUDES][1024];
    bool in_comment = false;
    bool update_line = false;
    int line = 0;
    int i = 0;
    int j = 0;
    int success;
    size_t buffsize;
    char *buffer;
    char *ptr;
    char in_string = 0;
    char includepath[2048];
    char actualpath[2048];
    FILE *f_source;

    current_operation = OP_PREPROCESS;
    strcpy(current_target, source);

    if (include_stack[i][0] == 0)
        fprintf(f_target, "# 1 \"%s\"\n", source);
    else
        fprintf(f_target, "# 1 \"%s\" 1\n", source);

    for (i = 0; i < MAXINCLUDES && include_stack[i][0] != 0; i++) {
        if (strcmp(source, include_stack[i]) == 0) {
            errorf("Circular dependency detected, printing include stack:\n", source);
            fprintf(stderr, "    !!! %s\n", source);
            for (j = MAXINCLUDES - 1; j >= 0; j--) {
                if (include_stack[j][0] == 0)
                    continue;
                fprintf(stderr, "        %s\n", include_stack[j]);
            }
            return 1;
        }
    }

    if (i == MAXINCLUDES) {
        errorf("Too many nested includes.\n");
        return 1;
    }

    strcpy(include_stack[i], source);

    f_source = fopen(source, "rb");
    if (!f_source) {
        errorf("Failed to open %s.\n", source);
        return 1;
    }

    // Skip byte order mark if it exists
    if (fgetc(f_source) == 0xef)
        fseek(f_source, 3, SEEK_SET);
    else
        fseek(f_source, 0, SEEK_SET);

    while (true) {
        // get line and add next lines if line ends with a backslash
        buffer = NULL;
        update_line = false;
        while (buffer == NULL || (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\\')) {
            if (buffer != NULL)
                update_line = true;

            ptr = NULL;
            buffsize = 0;
            if (getline(&ptr, &buffsize, f_source) == -1) {
                free(ptr);
                ptr = NULL;
                break;
            }

            line++;

            if (buffer == NULL) {
                buffer = ptr;
            } else {
                buffer = (char *)realloc(buffer, strlen(buffer) + 2 + buffsize);
                strcpy(buffer + strlen(buffer) - 2, ptr);
                free(ptr);
            }

            // Add trailing new line if necessary
            if (strlen(buffer) >= 1 && buffer[strlen(buffer) - 1] != '\n')
                strcat(buffer, "\n");

            // fix windows line endings
            if (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\r') {
                buffer[strlen(buffer) - 2] = '\n';
                buffer[strlen(buffer) - 1] = 0;
            }
        }

        if (buffer == NULL)
            break;

        // Check for block comment delimiters
        for (i = 0; i < strlen(buffer); i++) {
            if (in_string != 0) {
                if (buffer[i] == in_string && buffer[i-1] != '\\')
                    in_string = 0;
                else
                    continue;
            } else {
                if (!in_comment &&
                        (buffer[i] == '"' || buffer[i] == '\'') &&
                        (i == 0 || buffer[i-1] != '\\'))
                    in_string = buffer[i];
            }

            if (buffer[i] == '/' && buffer[i+1] == '/' && !in_comment) {
                buffer[i+1] = 0;
                buffer[i] = '\n';
            } else if (buffer[i] == '/' && buffer[i+1] == '*') {
                in_comment = true;
                buffer[i] = ' ';
                buffer[i+1] = ' ';
            } else if (buffer[i] == '*' && buffer[i+1] == '/') {
                in_comment = false;
                buffer[i] = ' ';
                buffer[i+1] = ' ';
            }

            if (in_comment && buffer[i] != '\n') {
                buffer[i] = ' ';
            }
        }

        // trim leading spaces
        trim_leading(buffer, strlen(buffer) + 1);

        if (!in_comment && strlen(buffer) >= 11 && strncmp(buffer, "#include", 8) == 0) {
            for (i = 0; i < strlen(buffer) ; i++) {
                if (buffer[i] == '<' || buffer[i] == '>')
                    buffer[i] = '"';
            }
            if (strchr(buffer, '"') == NULL) {
                errorf("Failed to parse #include in line %i in %s.\n", line, source);
                return 5;
            }
            strncpy(includepath, strchr(buffer, '"') + 1, sizeof(includepath));
            if (strchr(includepath, '"') == NULL) {
                errorf("Failed to parse #include in line %i in %s.\n", line, source);
                return 6;
            }
            *strchr(includepath, '"') = 0;
            if (find_file(includepath, source, actualpath)) {
                errorf("Failed to find %s.\n", includepath);
                return 7;
            }
            free(buffer);
            success = preprocess_prepare(actualpath, f_target, lineref);
            if (success)
                return success;

            current_operation = OP_PREPROCESS;
            strcpy(current_target, source);

            fprintf(f_target, "# %i \"%s\" 2\n", line + 1, source);

            for (i = 0; i < MAXINCLUDES && include_stack[i][0] != 0; i++);
            include_stack[i - 1][0] = 0;

            continue;
        } else {
            fputs(buffer, f_target);

            if (update_line)
                fprintf(f_target, "# %i \"%s\"\n", line + 1, source);
        }

        free(buffer);
    }

    fclose(f_source);

    return 0;
}


int preprocess(char *source, FILE *f_target, struct lineref *lineref) {
    extern char include_stack[MAXINCLUDES][1024];
    int i;
    int exitcode;
    int line;
    int current_file;
    char *buffer;
    char filename[1024];
    char temp1[1024];
    char temp2[1024];
    char command[1024];
    size_t buffsize;
    FILE *f_temp;

    if (get_temp_name(temp1, ".cpp") || get_temp_name(temp2, ".cpp")) {
        errorf("Failed to generate temp file.\n");
        return 1;
    }

    f_temp = fopen(temp1, "wb");
    if (!f_temp) {
        errorf("Failed to open temp file.\n");
        remove_file(temp1);
        remove_file(temp2);
        return 1;
    }

    for (i = 0; i < MAXINCLUDES; i++)
        include_stack[i][0] = 0;

    if (preprocess_prepare(source, f_temp, lineref)) {
        errorf("Failed to prepare file for preprocessing.\n");
        remove_file(temp1);
        remove_file(temp2);
        return 1;
    }

    fclose(f_temp);

    snprintf(command, sizeof(command), "gcc -E -o %s %s", temp2, temp1);
    exitcode = system(command);
    if (exitcode) {
        errorf("GCC returned exit code %i.\n", exitcode);
        //remove_file(temp1);
        remove_file(temp2);
        return 1;
    }

    if (remove_file(temp1)) {
        errorf("Failed to remove temp file.\n");
        return 1;
    }

    f_temp = fopen(temp2, "rb");
    if (!f_temp) {
        errorf("Failed to open temp file.\n");
        remove_file(temp2);
        return 1;
    }

    buffer = NULL;
    buffsize = 0;
    line = 0;
    while (getline(&buffer, &buffsize, f_temp) != -1) {
        if (buffer[0] == '#') {
            current_file = -1;
            sscanf(buffer, "# %i \"%1023c", &line, filename);
            if (strchr(filename, '"') != NULL)
                *(strchr(filename, '"')) = 0;

            for (i = 0; i < lineref->num_files; i++) {
                if (strcmp(lineref->file_names[i], filename) == 0) {
                    break;
                }
            }

            current_file = i;

            if (i == lineref->num_files) {
                strcpy(lineref->file_names[current_file], filename);
                lineref->num_files++;
            }
        } else {
            fputs(buffer, f_target);

            lineref->file_index[lineref->num_lines] = current_file;
            lineref->line_number[lineref->num_lines] = line;
            lineref->num_lines++;

            if (lineref->num_lines % LINEINTERVAL) {
                lineref->file_index = (uint32_t *)realloc(lineref->file_index, 4 * (lineref->num_lines + LINEINTERVAL));
                lineref->line_number = (uint32_t *)realloc(lineref->line_number, 4 * (lineref->num_lines + LINEINTERVAL));
            }
        }

        free(buffer);
        buffer = NULL;
        buffsize = 0;
        line++;
    }
    free(buffer);

    fclose(f_temp);
    if (remove_file(temp2)) {
        errorf("Failed to remove temp file.\n");
        return 1;
    }

    return 0;
}
