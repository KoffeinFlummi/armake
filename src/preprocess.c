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


char * resolve_macros(char *string, size_t buffsize, struct constant *constants) {
    /*
     * Returns NULL on failure and a pointer to the resolved string on success.
     */

    int i;
    int j;
    int level;
    char tmp[1024];
    char constant[1024];
    char replacement[262144];
    char argsearch[1024];
    char argquote[1024];
    char args[MAXARGS][1024];
    char *ptr;
    char *ptr_args;
    char *ptr_args_end;
    char *ptr_arg_end;
    char *ptr_temp;
    char in_string;
    size_t size_temp;

    for (i = 0; i < MAXCONSTS; i++) {
        if (constants[i].value == 0)
            break;
        if (constants[i].name[0] == 0) // this may be an undefed constant, so we have to keep going
            continue;
        if (strstr(string, constants[i].name) == NULL)
            continue;

        if (strcmp(constants[i].name, "__EVAL") == 0 || strcmp(constants[i].name, "__EXEC") == 0) {
            warningf("__EVAL and __EXEC macros are not supported.\n");
            continue;
        }

        ptr = string;
        while (true) {
            ptr = strstr(ptr, constants[i].name);
            if (ptr == NULL)
                break;

            // Check if start of constant invocation is correct
            if (ptr - string >= 2 && strncmp(ptr - 2, "##", 2) == 0) {
                snprintf(constant, sizeof(constant), "##%.*s", (int)strlen(constants[i].name), constants[i].name);
            } else if (ptr - string >= 1 && strncmp(ptr - 1, "#", 1) == 0) {
                snprintf(constant, sizeof(constant), "#%.*s", (int)strlen(constants[i].name), constants[i].name);
            } else if (ptr - string >= 1 && (
                    (*(ptr - 1) >= 'a' && *(ptr - 1) <= 'z') ||
                    (*(ptr - 1) >= 'A' && *(ptr - 1) <= 'Z') ||
                    (*(ptr - 1) >= '0' && *(ptr - 1) <= '9') ||
                    *(ptr - 1) == '_')) {
                ptr++;
                continue;
            } else {
                snprintf(constant, sizeof(constant), "%.*s", (int)strlen(constants[i].name), constants[i].name);
            }

            // Check if end of constant invocation is correct
            if (constants[i].arguments[0][0] == 0) {
                if (strlen(ptr + strlen(constants[i].name)) >= 2 &&
                        strncmp(ptr + strlen(constants[i].name), "##", 2) == 0) {
                    strncat(constant, "##", sizeof(constant) - strlen(constant));
                } else if (strlen(ptr + strlen(constants[i].name)) >= 1 && (
                        (*(ptr + strlen(constants[i].name)) >= 'a' && *(ptr + strlen(constants[i].name)) <= 'z') ||
                        (*(ptr + strlen(constants[i].name)) >= 'A' && *(ptr + strlen(constants[i].name)) <= 'Z') ||
                        (*(ptr + strlen(constants[i].name)) >= '0' && *(ptr + strlen(constants[i].name)) <= '9') ||
                        *(ptr + strlen(constants[i].name)) == '_')) {
                    ptr++;
                    continue;
                }
            } else {
                ptr_args = strchr(ptr, '(');
                ptr_args_end = strchr(ptr, ')');
                if (ptr_args == NULL || ptr_args_end == NULL || ptr_args > ptr_args_end) {
                    ptr++;
                    continue;
                }
                if (ptr + strlen(constants[i].name) != ptr_args) {
                    ptr++;
                    continue;
                }

                in_string = 0;
                level = 0;
                for (ptr_args_end = ptr_args + 1; *ptr_args_end != 0; ptr_args_end++) {
                    if (in_string != 0) {
                        if (*ptr_args_end == in_string && *(ptr_args_end - 1) != '\\')
                            in_string = 0;
                    } else if (level > 0) {
                        if (*ptr_args_end == '(')
                            level++;
                        if (*ptr_args_end == ')')
                            level--;
                    } else if ((*ptr_args_end == '"' || *ptr_args_end == '\'') && *(ptr_args_end - 1) != '\\') {
                        in_string = *ptr_args_end;
                    } else if (*ptr_args_end == '(') {
                        level++;
                    } else if (*ptr_args_end == ')') {
                        break;
                    }
                }

                strcpy(tmp, constant);
                snprintf(constant, sizeof(constant), "%s%.*s", tmp, (int)(ptr_args_end - ptr_args + 1), ptr_args);

                if (strlen(ptr + strlen(constants[i].name)) >= 2 &&
                        strncmp(ptr + strlen(constants[i].name), "##", 2) == 0) {
                    strncat(constant, "##", sizeof(constant) - strlen(constant));
                } else if (strlen(ptr_args_end) >= 1 && (
                        (*(ptr_args_end + 1) >= 'a' && *(ptr_args_end + 1) <= 'z') ||
                        (*(ptr_args_end + 1) >= 'A' && *(ptr_args_end + 1) <= 'Z') ||
                        (*(ptr_args_end + 1) >= '0' && *(ptr_args_end + 1) <= '9') ||
                        *(ptr_args_end + 1) == '_')) {
                    ptr++;
                    continue;
                }
            }

            strncpy(replacement, constants[i].value, sizeof(replacement));

            // Replace arguments in replacement string
            if (constants[i].arguments[0][0] != 0) {
                // extract values for arguments
                for (j = 0; j < MAXARGS && constants[i].arguments[j][0] != 0; j++) {
                    in_string = 0;
                    level = 0;
                    for (ptr_arg_end = ptr_args + 1; *ptr_arg_end != 0; ptr_arg_end++) {
                        if (ptr_arg_end > ptr_args_end)
                            return NULL;
                        if (in_string != 0) {
                            if (*ptr_arg_end == in_string && *(ptr_arg_end - 1) != '\\')
                                in_string = 0;
                        } else if (level > 0) {
                            if (*ptr_arg_end == '(')
                                level++;
                            if (*ptr_arg_end == ')')
                                level--;
                        } else if ((*ptr_arg_end == '"' || *ptr_arg_end == '\'') && *(ptr_arg_end - 1) != '\\') {
                            in_string = *ptr_arg_end;
                        } else if (*ptr_arg_end == '(') {
                            level++;
                        } else if (*ptr_arg_end == ')' || *ptr_arg_end == ',') {
                            strncpy(args[j], ptr_args + 1, ptr_arg_end - ptr_args - 1);
                            args[j][ptr_arg_end - ptr_args - 1] = 0;

                            trim(args[j], sizeof(args[j]));

                            if (resolve_macros(args[j], sizeof(args[j]), constants) == NULL)
                                return NULL;

                            ptr_args = ptr_arg_end;
                            break;
                        }
                    }
                }

                // replace arguments with values
                for (j = 0; j < MAXARGS && constants[i].arguments[j][0] != 0; j++) {
                    replace_string(args[j], sizeof(args[j]), constants[i].arguments[j], "\x11TOKENARG\x11", 0);

                    sprintf(argsearch, "##%s##", constants[i].arguments[j]);
                    replace_string(replacement, sizeof(replacement), argsearch, args[j], 0);
                    sprintf(argsearch, "%s##", constants[i].arguments[j]);
                    replace_string(replacement, sizeof(replacement), argsearch, args[j], 0);
                    sprintf(argsearch, "##%s", constants[i].arguments[j]);
                    replace_string(replacement, sizeof(replacement), argsearch, args[j], 0);

                    sprintf(argsearch, "#%s", constants[i].arguments[j]);
                    sprintf(argquote, "\"%s\"", args[j]);
                    replace_string(replacement, sizeof(replacement), argsearch, argquote, 0);

                    replace_string(replacement, sizeof(replacement), constants[i].arguments[j], args[j], 0);

                    replace_string(replacement, sizeof(replacement), "\x11TOKENARG\x11", constants[i].arguments[j], 0);
                }
            }

            if (resolve_macros(replacement, sizeof(replacement), constants) == NULL)
                return NULL;

            // realloc if necessary
            size_temp = buffsize;
            if (size_temp == 0) {
                size_temp = strlen(string) + 1;

                ptr_temp = string - 1;
                while ((ptr_temp = strstr(ptr_temp + 1, constant)) != NULL)
                    size_temp += strlen(replacement) - strlen(constant);

                if (size_temp > strlen(string) + 1)
                    string = (char *)realloc(string, size_temp);
                else
                    size_temp = strlen(string) + 1;
            }

            replace_string(string, size_temp, constant, replacement, 1);
        }
    }

    return string;
}


int preprocess(char *source, FILE *f_target, struct constant *constants, struct lineref *lineref) {
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
    int file_index;
    int line = 0;
    int i = 0;
    int j = 0;
    int level = 0;
    int level_true = 0;
    int level_comment = 0;
    int success;
    size_t buffsize;
    char *buffer;
    char *ptr;
    char *token;
    char in_string = 0;
    char valuebuffer[262144];
    char definition[256];
    char tmp[2048];
    char includepath[2048];
    char actualpath[2048];
    void *temp;
    FILE *f_source;

    current_operation = OP_PREPROCESS;
    strcpy(current_target, source);

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

    file_index = lineref->num_files;
    if (strchr(source, PATHSEP) == NULL)
        strcpy(lineref->file_names[file_index], source);
    else
        strcpy(lineref->file_names[file_index], strrchr(source, PATHSEP) + 1);

    lineref->num_files++;
    if (lineref->num_files >= MAXINCLUDES) {
        errorf("Number of included files exceeds MAXINCLUDES.\n");
        return 1;
    }

    // first constant is file name
    // @todo: what form?
    strcpy(constants[0].name, "__FILE__");
    if (constants[0].value == 0)
        constants[0].value = (char *)malloc(1024);
    snprintf(constants[0].value, 1024, "\"%s\"", source);

    strcpy(constants[1].name, "__LINE__");

    strcpy(constants[2].name, "__EXEC");
    if (constants[2].value == 0)
        constants[2].value = (char *)malloc(1);

    strcpy(constants[3].name, "__EVAL");
    if (constants[3].value == 0)
        constants[3].value = (char *)malloc(1);

    while (true) {
        // get line and add next lines if line ends with a backslash
        buffer = NULL;
        while (buffer == NULL || (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\\')) {
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
                if (level_comment == 0 &&
                        (buffer[i] == '"' || buffer[i] == '\'') &&
                        (i == 0 || buffer[i-1] != '\\'))
                    in_string = buffer[i];
            }

            if (buffer[i] == '/' && buffer[i+1] == '/') {
                buffer[i+1] = 0;
                buffer[i] = '\n';
            } else if (buffer[i] == '/' && buffer[i+1] == '*') {
                level_comment++;
                buffer[i] = ' ';
                buffer[i+1] = ' ';
            } else if (buffer[i] == '*' && buffer[i+1] == '/') {
                level_comment--;
                if (level_comment < 0)
                    level_comment = 0;
                buffer[i] = ' ';
                buffer[i+1] = ' ';
            }

            if (level_comment > 0) {
                buffer[i] = ' ';
                continue;
            }
        }

        // trim leading spaces
        trim_leading(buffer, strlen(buffer) + 1);

        // skip lines inside untrue ifs
        if (level > level_true) {
            if ((strlen(buffer) < 5 || strncmp(buffer, "#else", 5) != 0) &&
                    (strlen(buffer) < 6 || strncmp(buffer, "#endif", 6) != 0)) {
                free(buffer);
                continue;
            }
        }

        // second constant is line number
        if (constants[1].value == 0)
            constants[1].value = (char *)malloc(16);
        sprintf(constants[1].value, "%i", line - 1);

        // get the constant name
        if (strlen(buffer) >= 9 && (strncmp(buffer, "#define", 7) == 0 ||
                strncmp(buffer, "#undef", 6) == 0 ||
                strncmp(buffer, "#ifdef", 6) == 0 ||
                strncmp(buffer, "#ifndef", 7) == 0)) {
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
                errorf("Missing definition in line %i of %s.\n", line, source);
                return 2;
            }
        }

        // check for preprocessor commands
        if (level_comment == 0 && strlen(buffer) >= 9 && strncmp(buffer, "#define", 7) == 0) {
            i = 0;
            while (strlen(constants[i].name) != 0 &&
                    strcmp(constants[i].name, definition) != 0 &&
                    i < MAXCONSTS)
                i++;

            if (i == MAXCONSTS) {
                errorf("Maximum number of %i definitions exceeded in line %i of %s.\n", MAXCONSTS, line, source);
                return 3;
            }

            if (constants[i].name[0] != 0)
                nwarningf("redefinition-wo-undef", "Constant \"%s\" is being redefined without an #undef in line %i.\n", definition, line);

            ptr = buffer + strlen(definition) + 8;
            while (*ptr != ' ' && *ptr != '\t' && *ptr != '(' && *ptr != '\n')
                ptr++;

            // Get arguments and resolve macros in macro
            for (j = 0; j < MAXARGS; j++)
                constants[i].arguments[j][0] = 0;

            if (*ptr == '(' && strchr(ptr, ')') != NULL) {
                strncpy(tmp, ptr + 1, sizeof(tmp));

                if (strchr(tmp, ')') == NULL) {
                    errorf("Macro arguments too long in line %i of %s.\n", line, source);
                }
                *strchr(tmp, ')') = 0;

                token = strtok(tmp, ",");
                for (j = 0; j < MAXARGS && token; j++) {
                    strncpy(constants[i].arguments[j], token, sizeof(constants[i].arguments[j]));
                    trim(constants[i].arguments[j], sizeof(constants[i].arguments[j]));

                    token = strtok(NULL, ",");
                }

                ptr = strchr(ptr, ')') + 1;
            }

            while (*ptr == ' ' || *ptr == '\t')
                ptr++;

            if (*ptr != '\n') {
                strncpy(valuebuffer, ptr, sizeof(valuebuffer));
                valuebuffer[strlen(valuebuffer) - 1] = 0;

                if (resolve_macros(valuebuffer, sizeof(valuebuffer), constants) == NULL) {
                    errorf("Failed to resolve macros in line %i of %s.\n", line, source);
                    return 3;
                }

                if (strnlen(valuebuffer, sizeof(valuebuffer)) == sizeof(valuebuffer)) {
                    errorf("Macro value in line %i of %s exceeds maximum size (%i).\n", line, source, sizeof(valuebuffer));
                    return 3;
                }

                if (constants[i].value != 0)
                    free(constants[i].value);
                constants[i].value = (char *)malloc(strlen(valuebuffer) + 1);
                strcpy(constants[i].value, valuebuffer);
            } else {
                constants[i].value = (char *)malloc(1);
                constants[i].value[0] = 0;
            }

            strncpy(constants[i].name, definition, sizeof(constants[i].name));
        } else if (level_comment == 0 && strlen(buffer) >= 6 && strncmp(buffer, "#undef", 6) == 0) {
            i = 0;
            while (strcmp(constants[i].name, definition) != 0 &&
                    i < MAXCONSTS)
                i++;

            if (i < MAXCONSTS)
                constants[i].name[0] = 0;
        } else if (level_comment == 0 && strlen(buffer) >= 8 &&
                (strncmp(buffer, "#ifdef", 6) == 0 || strncmp(buffer, "#ifndef", 7) == 0)) {
            level++;
            if (strncmp(buffer, "#ifndef", 7) == 0)
                level_true++;
            for (i = 0; i < MAXCONSTS; i++) {
                if (strcmp(definition, constants[i].name) == 0) {
                    if (strncmp(buffer, "#ifdef", 6) == 0)
                        level_true++;
                    if (strncmp(buffer, "#ifndef", 7) == 0)
                        level_true--;
                }
            }
        } else if (level_comment == 0 && strlen(buffer) >= 5 && strncmp(buffer, "#else", 5) == 0) {
            if (level == level_true)
                level_true--;
            else
                level_true = level;
        } else if (level_comment == 0 && strlen(buffer) >= 6 && strncmp(buffer, "#endif", 6) == 0) {
            if (level == 0) {
                errorf("Unexpected #endif in line %i of %s.\n", line, source);
                return 4;
            }
            if (level == level_true)
                level_true--;
            level--;
        } else if (level_comment == 0 && strlen(buffer) >= 11 && strncmp(buffer, "#include", 8) == 0) {
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
            success = preprocess(actualpath, f_target, constants, lineref);
            if (success)
                return success;

            current_operation = OP_PREPROCESS;
            strcpy(current_target, source);

            for (i = 0; i < MAXINCLUDES && include_stack[i][0] != 0; i++);
            include_stack[i - 1][0] = 0;

            continue;
        }

        if (buffer[0] != '#' && strlen(buffer) > 1) {
            buffer = resolve_macros(buffer, 0, constants);
            if (buffer == NULL) {
                errorf("Failed to resolve macros in line %i of %s.\n", line, source);
                return success;
            }
            fputs(buffer, f_target);

            lineref->file_index[lineref->num_lines] = file_index;
            lineref->line_number[lineref->num_lines] = line;

            lineref->num_lines++;
            if (lineref->num_lines % LINEINTERVAL == 0) {
                temp = malloc(sizeof(uint32_t) * (lineref->num_lines + LINEINTERVAL));
                memcpy(temp, lineref->line_number, sizeof(uint32_t) * lineref->num_lines);
                free(lineref->line_number);
                lineref->line_number = (uint32_t *)temp;

                temp = malloc(sizeof(uint32_t) * (lineref->num_lines + LINEINTERVAL));
                memcpy(temp, lineref->file_index, sizeof(uint32_t) * lineref->num_lines);
                free(lineref->file_index);
                lineref->file_index = (uint32_t *)temp;
            }
        }
        free(buffer);
    }

    fclose(f_source);

    return 0;
}
