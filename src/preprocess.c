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


#define _GNU_SOURCE
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


#define IS_MACRO_CHAR(x) ( (x) == '_' || \
    ((x) >= 'a' && (x) <= 'z') || \
    ((x) >= 'A' && (x) <= 'Z') || \
    ((x) >= '0' && (x) <= '9') )


struct constants *constants_init() {
    struct constants *c = (struct constants *)malloc(sizeof(struct constants));
    c->head = NULL;
    c->tail = NULL;
    return c;
}

bool constants_parse(struct constants *constants, char *definition, int line) {
    struct constant *c = (struct constant *)malloc(sizeof(struct constant));
    char *ptr = definition;
    char *name;
    char *argstr;
    char *tok;
    char *start;
    char **args;
    int i;
    int len;
    bool quoted;

    while (IS_MACRO_CHAR(*ptr))
        ptr++;

    name = strndup(definition, ptr - definition);

    if (constants_remove(constants, name))
        nwarningf("redefinition-wo-undef", "Constant \"%s\" is being redefined without an #undef in line %i.\n", name, line);

    c->name = name;

    c->num_args = 0;
    if (*ptr == '(') {
        argstr = strdup(ptr + 1);
        if (strchr(argstr, ')') == NULL) {
            errorf("Missing ) in argument list of \"%s\".\n", c->name);
            return false;
        }
        *strchr(argstr, ')') = 0;
        ptr += strlen(argstr) + 2;

        args = (char **)malloc(sizeof(char *) * 4);

        tok = strtok(argstr, ",");
        while (tok) {
            if (c->num_args % 4 == 0)
                args = (char **)realloc(args, sizeof(char *) * (c->num_args + 4));
            args[c->num_args] = strdup(tok);
            trim(args[c->num_args], strlen(args[c->num_args]) + 1);
            c->num_args++;
            tok = strtok(NULL, ",");
        }

        free(argstr);
    }

    while (*ptr == ' ' || *ptr == '\t')
        ptr++;
    
    c->num_occurences = 0;
    if (c->num_args > 0) {
        c->occurrences = (int (*)[2])malloc(sizeof(int) * 4 * 2);
        c->value = strdup("");
        len = 0;

        while (true) {
            quoted = false;

            // Non-tokens
            start = ptr;
            while (*ptr != 0 && !IS_MACRO_CHAR(*ptr) && *ptr != '#')
                ptr++;

            while (len == 0 && (*start == ' ' || *start == '\t'))
                start++;

            if (ptr - start > 0) {
                len += ptr - start;
                c->value = (char *)realloc(c->value, len + 1);
                strncat(c->value, start, ptr - start);
            }

            quoted = *ptr == '#';
            if (quoted) {
                ptr++;
                if (*ptr == '#') {
                    errorf("Leading token concatenation operators (##) are not allowed or necessary.\n");
                    return false;
                }
            }

            if (*ptr == 0)
                break;

            // Potential tokens
            start = ptr;
            while (IS_MACRO_CHAR(*ptr))
                ptr++;

            tok = strndup(start, ptr - start);
            for (i = 0; i < c->num_args; i++) {
                if (strcmp(tok, args[i]) == 0)
                    break;
            }

            if (i == c->num_args) {
                if (quoted) {
                    errorf("Stringizing is only allowed for arguments.\n");
                    return false;
                }
                len += ptr - start;
                c->value = (char *)realloc(c->value, len + 1);
                strcat(c->value, tok);
            } else {
                if (c->num_occurences > 0 && c->num_occurences % 4 == 0)
                    c->occurrences = (int (*)[2])realloc(c->occurrences, sizeof(int) * 2 * (c->num_occurences + 4));
                c->occurrences[c->num_occurences][0] = i;

                if (quoted) {
                    c->occurrences[c->num_occurences][1] = len + 1;
                    len += 2;
                    c->value = (char *)realloc(c->value, len + 1);
                    strcat(c->value, "\"\"");
                } else {
                    c->occurrences[c->num_occurences][1] = len;
                }

                c->num_occurences++;
            }

            free(tok);

            // Handle concatenation
            if (*ptr == '#' && *(ptr + 1) == '#') {
                if (quoted) {
                    errorf("Token concatenations cannot be stringized.\n");
                    return false;
                }

                ptr += 2;
                if (!IS_MACRO_CHAR(*ptr)) {
                    errorf("Trailing token concatenation operators (##) are not allowed or necessary.\n");
                    return false;
                }
            }
        }

        ptr = c->value + (strlen(c->value) - 1);
        while ((c->num_occurences == 0 || c->occurrences[c->num_occurences - 1][1] < (ptr - c->value)) &&
                ptr > c->value && (*ptr == ' ' || *ptr == '\t'))
            ptr--;

        *(ptr + 1) = 0;
    } else {
        c->occurrences = NULL;
        c->value = strdup(ptr);
        trim(c->value, strlen(c->value) + 1);
    }

    c->last = constants->tail;
    c->next = NULL;
    if (constants->tail == NULL) {
        constants->head = constants->tail = c;
    } else {
        constants->tail->next = c;
        constants->tail = c;
    }

    if (c->num_args > 0) {
        for (i = 0; i < c->num_args; i++)
            free(args[i]);
        free(args);
    }

    return true;
}

bool constants_remove(struct constants *constants, char *name) {
    struct constant *c = constants_find(constants, name, 0);
    if (c == NULL)
        return false;

    if (c->next == NULL)
        constants->tail = c->last;
    else
        c->next->last = c->last;

    if (c->last == NULL)
        constants->head = c->next;
    else
        c->last->next = c->next;

    constant_free(c);

    return true;
}

struct constant *constants_find(struct constants *constants, char *name, int len) {
    struct constant *c = constants->head;

    while (c != NULL) {
        if (len <= 0 && strcmp(c->name, name) == 0)
            break;
        if (len > 0 && strlen(c->name) == len && strncmp(c->name, name, len) == 0)
            break;
        c = c->next;
    }

    return c;
}

char *constants_preprocess(struct constants *constants, char *source) {
    char *ptr = source;
    char *start;
    char *result;
    char *value;
    char **args;
    int i;
    int len = 0;
    int num_args;
    int level;
    char in_string;
    struct constant *c;

    result = (char *)malloc(1);
    result[0] = 0;

    while (true) {
        // Non-tokens
        start = ptr;
        while (*ptr != 0 && !IS_MACRO_CHAR(*ptr))
            ptr++;

        if (ptr - start > 0) {
            len += ptr - start;
            result = (char *)realloc(result, len + 1);
            strncat(result, start, ptr - start);
        }

        if (*ptr == 0)
            break;

        // Potential tokens
        start = ptr;
        while (IS_MACRO_CHAR(*ptr))
            ptr++;

        c = constants_find(constants, start, ptr - start);
        if (c == NULL || (c->num_args > 0 && *ptr != '(')) {
            len += ptr - start;
            result = (char *)realloc(result, len + 1);
            strncat(result, start, ptr - start);
            continue;
        }

        args = NULL;
        num_args = 0;
        if (*ptr == '(') {
            args = (char **)malloc(sizeof(char *) * 4);
            ptr++;
            start = ptr;

            in_string = 0;
            level = 0;
            while (*ptr != 0) {
                if (in_string) {
                    if (*ptr == in_string)
                        in_string = 0;
                } else if ((*ptr == '"' || *ptr == '\'') && *(ptr - 1) != '\\') {
                    in_string = *ptr;
                } else if (*ptr == '(') {
                    level++;
                } else if (level > 0 && *ptr == ')') {
                    level--;
                } else if (level == 0 && (*ptr == ',' || *ptr == ')')) {
                    if (num_args > 0 && num_args % 4 == 0)
                        args = (char **)realloc(args, sizeof(char *) * (num_args + 4));
                    args[num_args] = strndup(start, ptr - start);
                    num_args++;
                    if (*ptr == ')') {
                        break;
                    }
                    start = ptr + 1;
                }
                ptr++;
            }

            if (*ptr == 0) {
                errorf("Incomplete argument list for macro \"%s\".\n", c->name);
                return NULL;
            } else {
                ptr++;
            }
        }

        value = constant_value(constants, c, num_args, args);
        if (!value)
            return NULL;

        if (args) {
            for (i = 0; i < num_args; i++)
                free(args[i]);
            free(args);
        }

        len += strlen(value);
        result = (char *)realloc(result, len + 1);
        strcat(result, value);

        free(value);
    }

    free(source);
    
    return result;
}

void constants_free(struct constants *constants) {
    struct constant *c = constants->head;
    struct constant *next;

    while (c != NULL) {
        next = c->next;
        constant_free(c);
        c = next;
    }
    free(constants);
}

char *constant_value(struct constants *constants, struct constant *constant, int num_args, char **args) {
    int i;
    char *result;
    char *ptr;
    char *tmp;

    if (num_args != constant->num_args) {
        if (num_args)
            errorf("Macro \"%s\" expects %i arguments, %i given.\n", constant->name, constant->num_args, num_args);
        return NULL;
    }

    for (i = 0; i < num_args; i++) {
        args[i] = constants_preprocess(constants, args[i]);
        trim(args[i], strlen(args[i]) + 1);
        if (args[i] == NULL)
            return NULL;
    }

    if (num_args == 0) {
        result = strdup(constant->value);
    } else {
        result = strdup("");
        ptr = constant->value;
        for (i = 0; i < constant->num_occurences; i++) {
            tmp = strndup(ptr, constant->occurrences[i][1] - (ptr - constant->value));
            result = (char *)realloc(result, strlen(result) + strlen(tmp) + strlen(args[constant->occurrences[i][0]]) + 1);
            strcat(result, tmp);
            free(tmp);
            strcat(result, args[constant->occurrences[i][0]]);
            ptr = constant->value + constant->occurrences[i][1];
        }
        result = (char *)realloc(result, strlen(result) + strlen(ptr) + 1);
        strcat(result, ptr);
    }

    result = constants_preprocess(constants, result);
    trim(result, strlen(result) + 1);

    return result;
}

void constant_free(struct constant *constant) {
    free(constant->name);
    free(constant->value);
    if (constant->occurrences != NULL)
        free(constant->occurrences);
    free(constant);
}

bool is_number(char *var) {
    // [-+]?[0-9]+
    // [-+]?[0-9]*\.[0-9]+

    if (*var == '-' || *var == '+')
        var++;
    while (*var >= '0' && *var <= '9')
        var++;
    if (*var == '.') {
        var++;
        if (*var == 0)
            return false;
        while (*var != 0) {
            if (*var < '0' || *var > '9')
                return false;
            var++;
        }
    }
    return *var == 0;
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


int preprocess(char *source, FILE *f_target, struct constants *constants, struct lineref *lineref) {
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
    char *directive;
    char *directive_args;
    char in_string = 0;
    char includepath[2048];
    char actualpath[2048];
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
        fclose(f_source);
        return 1;
    }

    // first constant is file name
    // @todo
    // strcpy(constants[0].name, "__FILE__");
    // if (constants[0].value == 0)
    //     constants[0].value = (char *)malloc(1024);
    // snprintf(constants[0].value, 1024, "\"%s\"", source);

    // strcpy(constants[1].name, "__LINE__");

    // strcpy(constants[2].name, "__EXEC");
    // if (constants[2].value == 0)
    //     constants[2].value = (char *)malloc(1);

    // strcpy(constants[3].name, "__EVAL");
    // if (constants[3].value == 0)
    //     constants[3].value = (char *)malloc(1);

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
        // @todo
        // if (constants[1].value == 0)
        //     constants[1].value = (char *)malloc(16);
        // sprintf(constants[1].value, "%i", line - 1);

        if (level_comment == 0 && buffer[0] == '#') {
            ptr = buffer+1;
            while (*ptr == ' ' || *ptr == '\t')
                ptr++;

            directive = (char *)malloc(strlen(ptr) + 1);
            strcpy(directive, ptr);
            *(strchrnul(directive, ' ')) = 0;
            *(strchrnul(directive, '\n')) = 0;

            ptr += strlen(directive);
            while (*ptr == ' ' || *ptr == '\t')
                ptr++;
            directive_args = ptr;
            *(strchrnul(directive_args, '\n')) = 0;

            if (strcmp(directive, "include") == 0) {
                for (i = 0; i < strlen(directive_args) ; i++) {
                    if (directive_args[i] == '<' || directive_args[i] == '>')
                        directive_args[i] = '"';
                }
                if (strchr(directive_args, '"') == NULL) {
                    errorf("Failed to parse #include in line %i in %s.\n", line, source);
                    return 5;
                }
                strncpy(includepath, strchr(directive_args, '"') + 1, sizeof(includepath));
                if (strchr(includepath, '"') == NULL) {
                    errorf("Failed to parse #include in line %i in %s.\n", line, source);
                    return 6;
                }
                *strchr(includepath, '"') = 0;
                if (find_file(includepath, source, actualpath)) {
                    errorf("Failed to find %s.\n", includepath);
                    return 7;
                }

                free(directive);
                free(buffer);

                success = preprocess(actualpath, f_target, constants, lineref);

                for (i = 0; i < MAXINCLUDES && include_stack[i][0] != 0; i++);
                include_stack[i - 1][0] = 0;

                current_operation = OP_PREPROCESS;
                strcpy(current_target, source);

                if (success)
                    return success;
                continue;
            } else if (strcmp(directive, "define") == 0) {
                if (!constants_parse(constants, directive_args, line)) {
                    errorf("Failed to parse macro definition in line %i of %s.\n", line, source);
                    return 3;
                }
            } else if (strcmp(directive, "undef") == 0) {
                constants_remove(constants, directive_args);
            } else if (strcmp(directive, "ifdef") == 0) {
                level++;
                if (constants_find(constants, directive_args, 0))
                    level_true++;
            } else if (strcmp(directive, "ifndef") == 0) {
                level++;
                if (!constants_find(constants, directive_args, 0))
                    level_true++;
            } else if (strcmp(directive, "else") == 0) {
               if (level == level_true)
                   level_true--;
               else
                   level_true = level;
            } else if (strcmp(directive, "endif") == 0) {
               if (level == 0) {
                   errorf("Unexpected #endif in line %i of %s.\n", line, source);
                   return 4;
               }
               if (level == level_true)
                   level_true--;
               level--;
            } else {
                errorf("Unknown preprocessor directive \"%s\" in line %i of %s.\n", directive, line, source);
                return 5;
            }

            free(directive);
        } else if (strlen(buffer) > 1) {
            buffer = constants_preprocess(constants, buffer);
            if (buffer == NULL) {
                errorf("Failed to resolve macros in line %i of %s.\n", line, source);
                return success;
            }

            fputs(buffer, f_target);

            lineref->file_index[lineref->num_lines] = file_index;
            lineref->line_number[lineref->num_lines] = line;

            lineref->num_lines++;
            if (lineref->num_lines % LINEINTERVAL == 0) {
                lineref->file_index = (uint32_t *)realloc(lineref->file_index, 4 * (lineref->num_lines + LINEINTERVAL));
                lineref->line_number = (uint32_t *)realloc(lineref->line_number, 4 * (lineref->num_lines + LINEINTERVAL));
            }
        }

        free(buffer);
    }

    fclose(f_source);

    return 0;
}
