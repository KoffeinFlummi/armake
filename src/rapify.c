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
#endif

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"
#include "preprocess.h"
#include "rapify.h"


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

    buffer[0] = 0;
    while (strlen(buffer) < sizeof(buffer) - 1) {
        if (fgets(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), f_source) == NULL)
            break;
    }

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

    // remove trailing whitespace
    for (i = strlen(buffer) - 1; i > 0; i--) {
        if (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\n')
            buffer[i] = 0;
        else
            break;
    }

    // empty string
    if (strlen(buffer) == 0) {
        fwrite("\0\0", 2, 1, f_target);
        return 0;
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
    if (strlen(buffer) >= 2 && (buffer[0] == '"' || buffer[0] == '\'')) {
        if (buffer[0] != buffer[strlen(buffer) - 1])
            return 3;

        unescape_string(buffer, sizeof(buffer));
    } else if (buffer[0] != '"' && buffer[0] != '\'') {
        if (name == NULL)
            nwarningf("unquoted-string", "String in array is not quoted properly.\n");
        else
            nwarningf("unquoted-string", "String \"%s\" is not quoted properly.\n", name);
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

    fseek(f_source, 1, SEEK_CUR);

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

        if (current == '}' || current == ';')
            break;

        num_entries++;

        // go to next comma or end
        if (current == '{') {
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
        } else {
            in_string = 0;
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

                if (current == '}')
                    break;
                if (current == ',')
                    break;

                last = current;
                current = fgetc(f_source);
            }
        }

        if (current == '}')
            break;
    }

    write_compressed_int(num_entries, f_target);

    fseek(f_source, fp_tmp, SEEK_SET);

    while (true) {
        if (feof(f_source))
            return 4;
        if (skip_whitespace(f_source))
            return 5;

        fp_tmp = ftell(f_source);
        current = fgetc(f_source);
        fseek(f_source, fp_tmp, SEEK_SET);

        if (current == '}' || current == ',') {
            fseek(f_source, 1, SEEK_CUR);
            continue;
        }

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
    char name[512];
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
        if (strcmp(buffer, "class") != 0) {
            errorf("Expected \"class\", got \"%s\".\n", buffer);
            return 2;
        }

        fseek(f_source, 6, SEEK_CUR);
        fp_tmp = ftell(f_source);
        if (fgets(buffer, sizeof(buffer), f_source) == NULL)
            return 3;

        strncpy(name, buffer, sizeof(name));
        trim_leading(name, sizeof(name));
        for (ptr = name; *name != 0; ptr++) {
            if (*ptr == ' ' || *ptr == '\t' || *ptr == ':' || *ptr == '{') {
                *ptr = 0;
                break;
            }
        }

        while (strchr(buffer, '{') == NULL &&
                strlen(buffer) < sizeof(buffer) - 1) {
            if (fgets(buffer + strlen(buffer),
                    sizeof(buffer) - strlen(buffer), f_source) == NULL)
                return 4;
        }

        if (strchr(buffer, '{') == NULL) {
            errorf("Failed to find { for class \"%s\".\n", name);
            return 5;
        }

        if (strchr(buffer, ':') == NULL || strchr(buffer, '{') < strchr(buffer, ':')) {
            fputc(0, f_target);
        } else {
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
            if (is_root) {
                break;
            } else {
                errorf("Unexpected EOF in class \"%s\".\n", name);
                return 1;
            }
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
            if (is_root) {
                errorf("Unexpected end-of-class (}).\n");
                return 1;
            } else {
                break;
            }
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

        trim_leading(buffer, sizeof(buffer));
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

            while (strchr(buffer, ';') == NULL &&
                    strchr(buffer, '{') == NULL &&
                    strlen(buffer) < sizeof(buffer) - 1) {
                if (fgets(buffer + strlen(buffer),
                        sizeof(buffer) - strlen(buffer), f_source) == NULL)
                    return 4;
            }
            fseek(f_source, fp_tmp, SEEK_SET);

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

                    if (level < 0) {
                        errorf("Unexpected end-of-class (}).\n");
                        return 6;
                    }
                }

                continue;
            } else {
                errorf("This should never happen. If it does, you broke the laws of logic (or I am an idiot).\n");
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
        // @todo: ext, etc.?
        //if (is_root)
        //    return 2;

        // array
        if (strlen(buffer) > 2 && strcmp(buffer + strlen(buffer) - 2, "[]") == 0) {
            fseek(f_source, strlen(buffer), SEEK_CUR);

            buffer[strlen(buffer) - 2] = 0;

            if (skip_whitespace(f_source))
                return 3;
            if (feof(f_source))
                return 4;
            current = fgetc(f_source);
            if (current == '+') {
                fwrite("\x05\x01\0\0\0", 5, 1, f_target);
                if (fgetc(f_source) != '=') {
                    errorf("Expected \"=\" following \"+\".\n");
                    return 5;
                }
            } else if (current != '=') {
                errorf("Expected \"=\", got \"%c\".\n", current);
                return 5;
            } else {
                fputc(2, f_target);
            }
            if (skip_whitespace(f_source))
                return 6;

            fwrite(buffer, strlen(buffer) + 1, 1, f_target);

            success = rapify_array(f_source, f_target);
            if (success) {
                errorf("Failed to rapify array \"%s\".\n", buffer);
                return success;
            }

            // find trailing semicolon
            if (skip_whitespace(f_source))
                return 7;
            current = fgetc(f_source);
            if (current != ';') {
                errorf("Expected \";\", got \"%c\".\n", current);
                return 8;
            }

            continue;
        }

        // token
        fseek(f_source, strlen(buffer), SEEK_CUR);
        fputc(1, f_target);

        if (skip_whitespace(f_source))
            return 3;
        if (feof(f_source))
            return 4;
        current = fgetc(f_source);
        if (current != '=') {
            errorf("Expected \"=\" after \"%s\", got \"%c\".\n", buffer, current);
            return 5;
        }
        if (skip_whitespace(f_source))
            return 6;

        if (rapify_token(f_source, f_target, buffer)) {
            errorf("Failed to rapify token \"%s\".\n", buffer);
            return 7;
        }
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
                while (strchr(buffer, ';') == NULL &&
                        strchr(buffer, '{') == NULL &&
                        strlen(buffer) < sizeof(buffer) - 1) {
                    if (fgets(buffer + strlen(buffer),
                            sizeof(buffer) - strlen(buffer), f_source) == NULL)
                        return 10;
                }
                fseek(f_source, fp_tmp, SEEK_SET);

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

        fp_tmp = ftell(f_source);
        fseek(f_source, 6, SEEK_CUR);
        skip_whitespace(f_source);
        lookahead_word(f_source, name, sizeof(name));
        fseek(f_source, fp_tmp, SEEK_SET);

        success = rapify_class(f_source, f_target);
        if (success) {
            errorf("Failed to rapify class \"%s\".\n", name);
            return success;
        }
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

    FILE *f_temp;
    FILE *f_target;
    int i;
    int success;
    uint32_t enum_offset = 0;
    struct constant *constants;

    current_operation = OP_RAPIFY;
    strcpy(current_target, source);

#ifdef _WIN32
    char temp_name[2048];
    if (!GetTempFileName(getenv("HOMEPATH"), "amk", 0, temp_name)) {
        errorf("Failed to get temp file name.\n");
        return 1;
    }
    f_temp = fopen(temp_name, "w+");
#else
    f_temp = tmpfile();
#endif

    if (!f_temp) {
        errorf("Failed to open temp file.\n");
        return 1;
    }

    // Write original file to temp and pre process
    constants = (struct constant *)malloc(MAXCONSTS * sizeof(struct constant));
    for (i = 0; i < MAXCONSTS; i++) {
        constants[i].name[0] = 0;
        constants[i].arguments[0][0] = 0;
        constants[i].value = 0;
    }

    success = preprocess(source, f_temp, includefolder, constants);

    for (i = 0; i < MAXCONSTS && constants[i].value != 0; i++)
        free(constants[i].value);
    free(constants);

    current_operation = OP_RAPIFY;
    strcpy(current_target, source);

    if (success) {
        errorf("Failed to preprocess %s.\n", source);
        fclose(f_temp);
        return success;
    }

    // Rapify file
    fseek(f_temp, 0, SEEK_SET);
    f_target = fopen(target, "w");
    if (!f_target) {
        errorf("Failed to open %s.\n", target);
        fclose(f_temp);
        return 2;
    }
    fwrite("\0raP", 4, 1, f_target);
    fwrite("\0\0\0\0\x08\0\0\0", 8, 1, f_target);
    fwrite(&enum_offset, 4, 1, f_target); // this is replaced later

    success = rapify_class(f_temp, f_target);
    if (success) {
        errorf("Failed to rapify %s.\n", source);

        fclose(f_temp);
        fclose(f_target);

#ifdef _WIN32
        DeleteFile(temp_name);
#endif

        return success;
    }

    enum_offset = ftell(f_target);
    fwrite("\0\0\0\0", 4, 1, f_target); // fuck enums
    fseek(f_target, 12, SEEK_SET);
    fwrite(&enum_offset, 4, 1, f_target);

    fclose(f_temp);
    fclose(f_target);

#ifdef _WIN32
    DeleteFile(temp_name);
#endif

    return 0;
}
