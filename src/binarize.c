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
#include "binarize.h"


int mlod2odol(char *source, char *target) {
    return 0;
}


void get_word(char *target, char *source) {
    char *ptr;
    int len = 0;
    ptr = source;
    while (*ptr == ',' || *ptr == '(' || *ptr == ')' ||
            *ptr == ' ' || *ptr == '\t')
        ptr++;
    while (ptr[len] != ',' && ptr[len] != '(' && ptr[len] != ')' &&
            ptr[len] != ' ' && ptr[len] != '\t')
        len++;
    strncpy(target, ptr, len);
    target[len + 1] = 0;
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
     * file does not exist.
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


void replace_string(char *string, size_t buffsize, char *search, char *replace, int max) {
    /*
     * Replaces the search string with the given replacement in string.
     * max is the maximum number of occurrences to be replaced. 0 means
     * unlimited.
     */

    if (strstr(string, search) == NULL)
        return;

    char *tmp = malloc(buffsize);
    char *ptr;
    int i = 0;
    strncpy(tmp, string, buffsize);

    for (ptr = string; *ptr != 0; ptr++) {
        if (strlen(ptr) < strlen(search))
            break;

        if (strncmp(ptr, search, strlen(search)) == 0) {
            i++;
            strncpy(ptr, replace, buffsize - (ptr - string));
            ptr += strlen(replace);
            strncpy(ptr, tmp + (ptr - string) - (strlen(replace) - strlen(search)),
                    buffsize - (ptr - string));

            strncpy(tmp, string, buffsize);
        }

        if (max != 0 && i >= max)
            break;
    }

    free(tmp);
}


void quote(char *string) {
    char tmp[1024] = "\"";
    strncat(tmp, string, 1022);
    strcat(tmp, "\"");
    strncpy(string, tmp, 1024);
}


int resolve_macros(char *string, size_t buffsize, struct constant *constants) {
    int i;
    int j;
    int level;
    int success;
    char tmp[4096];
    char constant[4096];
    char replacement[4096];
    char argsearch[4096];
    char argvalue[4096];
    char argquote[4096];
    char args[MAXARGS][4096];
    char *ptr;
    char *ptr_args;
    char *ptr_args_end;
    char *ptr_arg_end;
    char in_string;

    for (i = 0; i < MAXCONSTS; i++) {
        if (constants[i].name[0] == 0)
            continue;
        if (constants[i].value[0] == 0)
            continue;
        if (strstr(string, constants[i].name) == NULL)
            continue;

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
                            return 2;
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
                            ptr_args = ptr_arg_end;
                            break;
                        }
                    }
                }

                // replace arguments with values
                for (j = 0; j < MAXARGS && constants[i].arguments[j][0] != 0; j++) {
                    strncpy(argvalue, args[j], sizeof(argvalue));
                    replace_string(argvalue, sizeof(argvalue), constants[i].arguments[j], "\%\%\%TOKENARG\%\%\%", 0);

                    sprintf(argsearch, "##%s##", constants[i].arguments[j]);
                    replace_string(replacement, sizeof(replacement), argsearch, argvalue, 0);
                    sprintf(argsearch, "%s##", constants[i].arguments[j]);
                    replace_string(replacement, sizeof(replacement), argsearch, argvalue, 0);
                    sprintf(argsearch, "##%s", constants[i].arguments[j]);
                    replace_string(replacement, sizeof(replacement), argsearch, argvalue, 0);

                    sprintf(argsearch, "#%s", constants[i].arguments[j]);
                    sprintf(argquote, "\"%s\"", argvalue);
                    replace_string(replacement, sizeof(replacement), argsearch, argquote, 0);

                    replace_string(replacement, sizeof(replacement), constants[i].arguments[j], argvalue, 0);

                    replace_string(replacement, sizeof(replacement), "\%\%\%TOKENARG\%\%\%", constants[i].arguments[j], 0);
                }
            }

            success = resolve_macros(replacement, sizeof(replacement), constants);
            if (success)
                return success;

            replace_string(string, buffsize, constant, replacement, 1);
        }
    }

    return 0;
}


int preprocess(char *source, FILE *f_target, char *includefolder, struct constant *constants) {
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
     */

    int line = 0;
    int i = 0;
    int j = 0;
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

    // first constant is file name
    // @todo: what form?
    strcpy(constants[0].name, "__FILE__");
    snprintf(constants[0].value, sizeof(constants[0].value), "\"%s\"", source);

    while (true) {
        // get line
        line++;
        buffsize = 4096;
        buffer = (char *)malloc(buffsize + 1);
        if (getline(&buffer, &buffsize, f_source) == -1)
            break;

        // fix Windows line endings
        if (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\r') {
            buffer[strlen(buffer) - 2] = '\n';
            buffer[strlen(buffer) - 1] = 0;
        }

        // add next lines if line ends with a backslash
        while (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\\') {
            buffsize -= strlen(buffer);
            ptr = (char *)malloc(buffsize + 1);
            if (getline(&ptr, &buffsize, f_source) == -1)
                break;
            strncpy(strrchr(buffer, '\\'), ptr, buffsize);
            line++;
            free(ptr);

            // fix windows line endings again
            if (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\r') {
                buffer[strlen(buffer) - 2] = '\n';
                buffer[strlen(buffer) - 1] = 0;
            }
        }

        // remove line comments
        if (strstr(buffer, "//") != NULL) {
            *(strstr(buffer, "//") + 1) = 0;
            buffer[strlen(buffer) - 1] = '\n';
        }

        // Check for block comment delimiters
        for (i = 0; i < strlen(buffer); i++) {
            if (i == strlen(buffer) - 1) {
                if (level_comment > 0)
                    buffer[i] = ' ';
                break;
            }

            if (buffer[i] == '/' && buffer[i+1] == '*') {
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
            if (level_comment > 0)
                buffer[i] = ' ';
        }

        // trim leading spaces
        trim_leading(buffer);

        // skip lines inside untrue ifs
        if (level > level_true) {
            if ((strlen(buffer) < 5 || strncmp(buffer, "#else", 5) != 0) &&
                    (strlen(buffer) < 6 || strncmp(buffer, "#endif", 6) != 0)) {
                free(buffer);
                continue;
            }
        }

        // second constant is line number
        strcpy(constants[1].name, "__LINE__");
        sprintf(constants[1].value, "%i", line);

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
                printf("Missing definition in line %i of %s.\n", line, source);
                return 2;
            }
        }

        // check for preprocessor commands
        if (level_comment == 0 && strlen(buffer) >= 9 && strncmp(buffer, "#define", 7) == 0) {
            i = 0;
            while (strlen(constants[i].name) != 0 &&
                    strcmp(constants[i].name, definition) != 0 &&
                    i <= MAXCONSTS)
                i++;

            if (i == MAXCONSTS) {
                printf("Maximum number of %i definitions exceeded in line %i of %s.\n", MAXCONSTS, line, source);
                return 3;
            }

            ptr = buffer + strlen(definition) + 8;
            while (*ptr != ' ' && *ptr != '\t' && *ptr != '(' && *ptr != '\n')
                ptr++;

            // Get arguments and resolve macros in macro
            constants[i].arguments[0][0] = 0;
            if (*ptr == '(' && strchr(ptr, ')') != NULL) {
                for (j = 0; j < MAXARGS; j++) {
                    get_word(constants[i].arguments[j], ptr);
                    if (strchr(ptr, ',') == NULL || strchr(ptr, ',') > strchr(ptr, ')')) {
                        ptr = strchr(ptr, ')') + 1;
                        break;
                    }
                    ptr = strchr(ptr, ',') + 1;
                }
                if (j + 1 < MAXARGS)
                    constants[i].arguments[j + 1][0] = 0;
            }

            while (*ptr == ' ' || *ptr == '\t')
                ptr++;

            if (*ptr != '\n') {
                strncpy(constants[i].value, ptr, 4096);
                constants[i].value[strlen(constants[i].value) - 1] = 0;

                success = resolve_macros(constants[i].value, sizeof(constants[i].value), constants);
                if (success) {
                    printf("Failed to resolve macros in line %i of %s.\n", line, source);
                    return success;
                }
            } else {
                constants[i].value[0] = 0;
            }

            strncpy(constants[i].name, definition, 256);
        } else if (level_comment == 0 && strlen(buffer) >= 6 && strncmp(buffer, "#undef", 6) == 0) {
            i = 0;
            while (strlen(constants[i].name) != 0 &&
                    strcmp(constants[i].name, definition) != 0 &&
                    i <= MAXCONSTS)
                i++;

            if (i == MAXCONSTS) {
                printf("Include %s not found in line %i of %s.\n", definition, line, source);
                return 3;
            }

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
                printf("Unexpected #endif in line %i of %s.\n", line, source);
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
            success = preprocess(actualpath, f_target, includefolder, constants);
            if (success)
                return success;
            continue;
        }

        if (buffer[0] != '#' && strlen(buffer) > 1) {
            success = resolve_macros(buffer, 4096, constants);
            if (success) {
                printf("Failed to resolve macros in line %i of %s.\n", line, source);
                return success;
            }
            fputs(buffer, f_target);
        }
        free(buffer);
    }

    fclose(f_source);

    return 0;
}


char lookahead_c(FILE *f) {
    /*
     * Gets the next character for the given file pointer without changing
     * the file pointer position.
     *
     * Returns a char on success, -1 on failure.
     */

    char result;
    int fpos;

    if (feof(f))
        return -1;

    fpos = ftell(f);
    result = fgetc(f);
    fseek(f, fpos, SEEK_SET);

    return result;
}


int lookahead_word(FILE *f, char *buffer, size_t buffsize) {
    /*
     * Gets the next word for the given file pointer without changing the
     * file pointer position.
     *
     * Returns 0 on success, a positive integer on failure.
     */

    int fpos;
    int i;

    if (feof(f))
        return 1;

    fpos = ftell(f);
    if (fgets(buffer, buffsize, f) == NULL)
        return 2;
    fseek(f, fpos, SEEK_SET);

    for (i = 0; i < buffsize; i++) {
        if (buffer[i] == 0)
            return 3;
        if (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\n' ||
                buffer[i] == ',' || buffer[i] == ';' || buffer[i] == '{' ||
                buffer[i] == '}' || buffer[i] == '(' || buffer[i] == ')' ||
                buffer[i] == '=') {
            buffer[i] = 0;
            break;
        }
    }

    if (i == buffsize)
        return 4;

    return 0;
}


int skip_whitespace(FILE *f) {
    /*
     * Advances the pointer to the next non-whitespace character.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    char current;

    do {
        if (feof(f))
            return 1;
        current = fgetc(f);
    } while (current == ' ' || current == '\t' || current == '\n');

    fseek(f, -1, SEEK_CUR);
    return 0;
}


void unescape_string(char *buffer, size_t buffsize) {
    char *tmp;
    char *ptr;
    char tmp_array[2];
    char current;

    tmp = malloc(buffsize);
    tmp[0] = 0;
    tmp_array[1] = 0;

    buffer[strlen(buffer) - 1] = 0;
    for (ptr = buffer + 1; *ptr != 0; ptr++) {
        current = *ptr;

        if (*ptr == '\\' && *(ptr + 1) == '\\') {
            ptr++;
        } else if (*ptr == '\\' && *(ptr + 1) == 'n') {
            current = '\n';
            ptr++;
        } else if (*ptr == '\\' && *(ptr + 1) == 'r') {
            current = '\r';
            ptr++;
        } else if (*ptr == '\\' && *(ptr + 1) == 't') {
            current = '\t';
            ptr++;
        } else if (*ptr == '\\' && *(ptr + 1) == '"') {
            current = '"';
            ptr++;
        } else if (*ptr == '\\' && *(ptr + 1) == '\'') {
            current = '\'';
            ptr++;
        } else if (*ptr == '"' && *(ptr + 1) == '"') {
            ptr++;
        } else if (*ptr == '\'' && *(ptr + 1) == '\'') {
            ptr++;
        }

        if (*ptr == 0)
            break;

        tmp_array[0] = current;
        strcat(tmp, tmp_array);
    }

    strcpy(buffer, tmp);

    free(tmp);
}


void write_compressed_int(uint32_t integer, FILE *f_target) {
    uint64_t tmp;
    char c;

    tmp = (uint64_t)integer;

    if (tmp == 0) {
        fwrite(&tmp, 1, 1, f_target);
    }

    while (tmp > 0) {
        if (tmp > 0x7f) {
            // there are going to be more entries
            c = 0x80 | (tmp & 0x7f);
            fwrite(&c, 1, 1, f_target);
            tmp = tmp >> 7;
        } else {
            // last entry
            c = tmp;
            fwrite(&c, 1, 1, f_target);
            tmp = 0;
        }
    }
}


int rapify_token(FILE *f_source, FILE *f_target, char *name) {
    int i;
    uint32_t fp_tmp;
    uint32_t value_long;
    float value_float;
    char last;
    char current;
    char in_string;
    char buffer[4096];
    char *endptr;

    fp_tmp = ftell(f_source);

    if (fgets(buffer, sizeof(buffer), f_source) == NULL)
        return 1;

    // find trailing semicolon (or comma or } for arrays)
    in_string = 0;
    current = 0;
    for (i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] == 0)
            return 2;

        last = current;
        current = buffer[i];

        if (in_string != 0) {
            if (current == in_string && last != '\\')
                in_string = 0;
            else
                continue;
        } else {
            if ((current == '"' || current == '\'') && last != '\\')
                in_string = current;
        }

        if ((name != NULL && current == ';') ||
                (name == NULL && (current == ',' || current == '}'))) {
            buffer[i] = 0;
            break;
        }
    }

    fseek(f_source, fp_tmp + strlen(buffer) + 1, SEEK_SET);

    // empty string
    if (strlen(buffer) == 0) {
        fwrite("\0\0", 2, 1, f_target);
        return 0;
    }

    // remove trailing whitespace
    for (i = strlen(buffer) - 1; i < 0; i--) {
        if (buffer[i] == ' ' || buffer[i] == '\t')
            buffer[i] = 0;
    }

    // try the value as long int
    value_long = strtol(buffer, &endptr, 0);
    if (strlen(endptr) == 0) {
        fputc(2, f_target);
        if (name != NULL)
            fwrite(name, strlen(name) + 1, 1, f_target);
        fwrite(&value_long, 4, 1, f_target);
        return 0;
    }

    // try the value as float
    value_float = strtof(buffer, &endptr);
    if (strlen(endptr) == 0) {
        fputc(1, f_target);
        if (name != NULL)
            fwrite(name, strlen(name) + 1, 1, f_target);
        fwrite(&value_float, 4, 1, f_target);
        return 0;
    }

    // it's a string.
    fputc(0, f_target);
    if (name != NULL)
        fwrite(name, strlen(name) + 1, 1, f_target);

    // unescape only if it's written as a proper string
    if (strlen(buffer) >= 2 && (buffer[i] == '"' || buffer[i] == '\'')) {
        if (buffer[0] != buffer[strlen(buffer) - 1])
            return 3;

        unescape_string(buffer, sizeof(buffer));
    }

    fwrite(buffer, strlen(buffer) + 1, 1, f_target);

    return 0;
}


int rapify_array(FILE *f_source, FILE *f_target) {
    int level;
    int success;
    char last;
    char current;
    char in_string;
    uint32_t fp_tmp;
    uint32_t num_entries;

    fp_tmp = ftell(f_source);

    num_entries = 0;
    in_string = 0;
    level = 0;
    while (true) {
        if (feof(f_source))
            return 1;
        if (skip_whitespace(f_source))
            return 2;

        last = 0;
        current = fgetc(f_source);

        if (current == ';')
            break;

        if (current != ',' && current != '{') {
            num_entries++;

            // go to next comma or end
            in_string = 0;
            level = 0;
            while (true) {
                if (feof(f_source))
                    return 3;

                if (in_string != 0) {
                    if (current == in_string && last != '\\') {
                        in_string = 0;
                    } else {
                        last = current;
                        current = fgetc(f_source);
                        continue;
                    }
                } else {
                    if ((current == '"' || current == '\'') && last != '\\')
                        in_string = current;
                }

                if (current == '{')
                    level++;
                else if (current == '}')
                    level--;

                if (level < 0)
                    break;
                if (level == 0 && current == ',')
                    break;

                last = current;
                current = fgetc(f_source);
            }
        }
    }

    write_compressed_int(num_entries, f_target);

    fseek(f_source, fp_tmp + 1, SEEK_SET);

    while (true) {
        if (feof(f_source))
            return 4;
        if (skip_whitespace(f_source))
            return 5;

        fp_tmp = ftell(f_source);
        current = fgetc(f_source);
        fseek(f_source, fp_tmp, SEEK_SET);

        if (current == ';')
            break;

        if (current == '{') {
            fputc(3, f_target);
            success = rapify_array(f_source, f_target);
            if (success)
                return success;
        } else {
            success = rapify_token(f_source, f_target, NULL);
            if (success)
                return success;
        }
    }

    return 0;
}


int rapify_class(FILE *f_source, FILE *f_target) {
    bool is_root;
    int i;
    int success = 0;
    int level = 0;
    char in_string = 0;
    char current = 0;
    char last = 0;
    char buffer[4096];
    char tmp[4096];
    char *ptr;
    uint32_t fp_start;
    uint32_t fp_tmp;
    uint32_t fp_endaddress;
    uint32_t classheaders[MAXCLASSES];
    uint32_t num_entries = 0;

    is_root = (ftell(f_target) <= 16);

    for (i = 0; i < MAXCLASSES; i++)
        classheaders[i] = 0;

    // if this is the first entry, write file "class"
    if (is_root) {
        fputc(0, f_target); // inherited class
    } else {
        if (lookahead_word(f_source, buffer, sizeof(buffer)))
            return 1;
        if (strcmp(buffer, "class") != 0)
            return 2;

        fseek(f_source, 6, SEEK_CUR);
        fp_tmp = ftell(f_source);
        if (fgets(buffer, sizeof(buffer), f_source) == NULL)
            return 3;

        if (strchr(buffer, '{') == NULL)
            return 4;

        if (strchr(buffer, ':') == NULL || strchr(buffer, '{') < strchr(buffer, ':')) {
            fputc(0, f_target);
        } else {
            if (strchr(buffer, ':') > strchr(buffer, '{'))
                return 5;

            ptr = strchr(buffer, ':') + 1;
            while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n')
                ptr++;
            strcpy(tmp, ptr);

            ptr = tmp;
            while (*ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != '{')
                ptr++;
            *ptr = 0;

            fwrite(tmp, strlen(tmp) + 1, 1, f_target);
        }

        fseek(f_source, fp_tmp + (strchr(buffer, '{') - buffer) + 1, SEEK_SET);
    }

    fp_start = ftell(f_source);

    // FIRST ITERATION: get number of entries
    while (true) {
        if (feof(f_source)) {
            if (is_root)
                break;
            else
                return 1;
        }

        last = current;
        current = fgetc(f_source);

        if (in_string != 0) {
            if (current == in_string && last != '\\')
                in_string = 0;
            else
                continue;
        } else {
            if ((current == '"' || current == '\'') && last != '\\')
                in_string = current;
        }

        if (current == '{')
            level++;
        else if (current == '}')
            level--;

        if (level < 0) {
            if (is_root)
                return 1;
            else
                break;
        }

        if (level == 0 && current == ';')
            num_entries++;
    }

    write_compressed_int(num_entries, f_target);

    fseek(f_source, fp_start, SEEK_SET);

    // SECOND ITERATION: write entries and class headers to file
    while (true) {
        if (skip_whitespace(f_source))
            break;

        if (lookahead_word(f_source, buffer, sizeof(buffer)))
            return 1;

        if (strlen(buffer) == 0)
            break;

        // class definitions
        if (strcmp(buffer, "class") == 0) {
            fseek(f_source, 5, SEEK_CUR);
            if (skip_whitespace(f_source))
                return 2;

            fp_tmp = ftell(f_source);
            if (fgets(buffer, sizeof(buffer), f_source) == NULL)
                return 3;
            fseek(f_source, fp_tmp, SEEK_SET);

            if (strchr(buffer, ';') == NULL && strchr(buffer, '{') == NULL)
                return 4;

            if (strchr(buffer, '{') == NULL || (strchr(buffer, ';') != NULL &&
                    strchr(buffer, ';') < strchr(buffer, '{'))) {
                // extern class definition
                fputc(3, f_target);

                ptr = buffer;
                while (*ptr != ' ' && *ptr != '\t' && *ptr != ';')
                    ptr++;
                *ptr = 0;
                fwrite(buffer, strlen(buffer) + 1, 1, f_target);

                // wkip to trailing semicolon
                while (true) {
                    if (feof(f_source))
                        return 5;
                    if (fgetc(f_source) == ';')
                        break;
                }

                continue;
            } else if (strchr(buffer, ';') == NULL || (strchr(buffer, '{') != NULL &&
                    strchr(buffer, ';') > strchr(buffer, '{'))) {
                // class body definition
                fputc(0, f_target);

                ptr = buffer;
                while (*ptr != ' ' && *ptr != '\t' && *ptr != '{' && *ptr != ':')
                    ptr++;
                *ptr = 0;
                fwrite(buffer, strlen(buffer) + 1, 1, f_target);

                for (i = 0; i < MAXCLASSES && classheaders[i] != 0; i++);
                classheaders[i] = ftell(f_target);
                fwrite("\0\0\0\0", 4, 1, f_target);

                // wkip to trailing semicolon
                in_string = 0;
                level = 0;
                while (true) {
                    if (feof(f_source))
                        return 5;

                    last = current;
                    current = fgetc(f_source);

                    if (in_string != 0) {
                        if (current == in_string && last != '\\')
                            in_string = 0;
                        else
                            continue;
                    } else {
                        if ((current == '"' || current == '\'') && last != '\\')
                            in_string = current;
                    }

                    if (current == '{')
                        level++;
                    else if (current == '}')
                        level--;

                    if (level == 0 && current == ';')
                        break;

                    if (level < 0)
                        return 6;
                }

                continue;
            } else {
                return 7;
            }
        }

        // delete statement
        if (strcmp(buffer, "delete") == 0) {
            fputc(4, f_target);

            if (lookahead_word(f_source, buffer, sizeof(buffer)))
                return 2;

            fwrite(buffer, strlen(buffer) + 1, 1, f_target);

            // wkip to trailing semicolon
            while (true) {
                if (feof(f_source))
                    return 3;
                if (fgetc(f_source) == ';')
                    break;
            }

            continue;
        }

        // on the root level, nothing but classes is allowed
        if (is_root)
            return 2;

        // array
        if (strcmp(buffer + strlen(buffer) - 2, "[]") == 0) {
            fseek(f_source, strlen(buffer), SEEK_CUR);

            fputc(2, f_target);
            buffer[strlen(buffer) - 2] = 0;
            fwrite(buffer, strlen(buffer) + 1, 1, f_target);

            if (skip_whitespace(f_source))
                return 3;
            if (feof(f_source))
                return 4;
            if (fgetc(f_source) != '=')
                return 5;
            if (skip_whitespace(f_source))
                return 6;

            success = rapify_array(f_source, f_target);
            if (success)
                return success;

            // find trailing semicolon
            if (skip_whitespace(f_source))
                return 7;
            if (fgetc(f_source) != ';')
                return 8;

            continue;
        }

        // token
        fseek(f_source, strlen(buffer), SEEK_CUR);
        fputc(1, f_target);

        if (skip_whitespace(f_source))
            return 3;
        if (feof(f_source))
            return 4;
        if (fgetc(f_source) != '=')
            return 5;
        if (skip_whitespace(f_source))
            return 6;

        if (rapify_token(f_source, f_target, buffer))
            return 7;
    }

    fp_endaddress = ftell(f_target);
    fwrite("\0\0\0\0", 4, 1, f_target);

    fseek(f_source, fp_start, SEEK_SET);

    // THIRD ITERACTION: write class bodies to file recursively
    for (i = 0; i < MAXCLASSES && classheaders[i] != 0; i++) {
        while (true) {
            if (skip_whitespace(f_source))
                return 7;
            if (lookahead_word(f_source, buffer, sizeof(buffer)))
                return 8;

            if (strcmp(buffer, "class") == 0) {
                fp_tmp = ftell(f_source);
                if (fgets(buffer, sizeof(buffer), f_source) == NULL)
                    return 9;
                fseek(f_source, fp_tmp, SEEK_SET);

                if (strchr(buffer, '{') == NULL && strchr(buffer, ';') == NULL)
                    return 10;

                if (strchr(buffer, ';') == NULL)
                    break;
                if (strchr(buffer, '}') != NULL && strchr(buffer, '{') < strchr(buffer, ';'))
                    break;

                strcpy(buffer, "class"); // reset buffer
            }

            if (strlen(buffer) == 0)
                fseek(f_source, 1, SEEK_CUR);
            else
                fseek(f_source, strlen(buffer), SEEK_CUR);
        }

        fp_tmp = ftell(f_target);
        fseek(f_target, classheaders[i], SEEK_SET);
        fwrite(&fp_tmp, 4, 1, f_target);
        fseek(f_target, 0, SEEK_END);

        success = rapify_class(f_source, f_target);
        if (success)
            return success;
    }

    // write end address
    fp_tmp = ftell(f_target);
    fseek(f_target, fp_endaddress, SEEK_SET);
    fwrite(&fp_tmp, 4, 1, f_target);
    fseek(f_target, 0, SEEK_END);

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
    FILE *f_target;
    int i;
    int success;
    uint32_t enum_offset = 0;
    struct constant *constants;

    // Write original file to temp and pre process
    constants = (struct constant *)malloc(MAXCONSTS * sizeof(struct constant));
    for (i = 0; i < MAXCONSTS; i++) {
        constants[i].name[0] = 0;
        constants[i].arguments[0][0] = 0;
        constants[i].value[0] = 0;
    }
    success = preprocess(source, f_temp, includefolder, constants);
    free(constants);
    if (success) {
        printf("Failed to preprocess %s.\n", source);
        fclose(f_temp);
        return success;
    }

    // Rapify file
    fseek(f_temp, 0, SEEK_SET);
    f_target = fopen(target, "w");
    if (!f_target) {
        printf("Failed to open %s.\n", target);
        fclose(f_temp);
        return 2;
    }
    fwrite("\0raP", 4, 1, f_target);
    fwrite("\0\0\0\0\x08\0\0\0", 8, 1, f_target);
    fwrite(&enum_offset, 4, 1, f_target); // this is replaced later

    success = rapify_class(f_temp, f_target);
    if (success) {
        printf("Failed to rapify %s.\n", source);
        fclose(f_temp);
        fclose(f_target);
        return success;
    }

    enum_offset = ftell(f_target);
    fwrite("\0\0\0\0", 4, 1, f_target); // fuck enums
    fseek(f_target, 12, SEEK_SET);
    fwrite(&enum_offset, 4, 1, f_target);

    fclose(f_temp);
    fclose(f_target);
    return 0;
}


int binarize_file(char *source, char *target, char *includefolder) {
    /*
     * Binarize the given file. If source and target are identical, the target
     * is overwritten. If the source is a P3D, it is converted to ODOL. If the
     * source is a rapifiable type (cpp, ext, etc.), it is rapified.
     *
     * If the file type is not recognized, -1 is returned. 0 is returned on
     * success and a positive integer on error.
     *
     * The include folder argument is the path to be searched for file
     * inclusions.
     */

    char fileext[64];

    if (strchr(source, '.') == NULL)
        return -1;

    strncpy(fileext, strrchr(source, '.'), 64);

    if (!strcmp(fileext, ".cpp") ||
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
