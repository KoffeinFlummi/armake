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
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"
#include "preprocess.h"
#include "rapify.h"


#define SKIP_WHITESPACE(fp) if (skip_whitespace(fp)) { errorf("Unexpected EOF.\n"); return 1; }

#define LOOKAHEAD_WORD(fp, buffer) \
    if (lookahead_word(fp, buffer, sizeof(buffer))) { \
        errorf("Unexpected EOF.\n"); \
        return 6; \
    }
#define LOOKAHEAD_C(fp, var) \
    var = lookahead_c(fp); \
    if (var == -1) { \
        errorf("Unexpected EOF.\n"); \
        return 7; \
    }

#define GET_WORD(fp, buffer) \
    LOOKAHEAD_WORD(fp, buffer); \
    fseek(fp, strlen(buffer), SEEK_CUR);
#define GET_C(fp, var) \
    if (feof(fp)) { \
        errorf("Unexpected EOF.\n"); \
        return 4; \
    } \
    var = fgetc(fp);

#define EXPECT_WORD(fp, buffer, word, lineref) \
    if (strcmp(buffer, word) != 0) { \
        errorf("Expected \"%s\", got \"%s\" in %s:%i.\n", \
            word, buffer, lineref->file_names[lineref->file_index[get_line_number(fp)]], \
            lineref->line_number[get_line_number(fp)]); \
        return 3; \
    }
#define EXPECT_C(fp, var, c, lineref) \
    if (var != c) { \
        errorf("Expected '%c', got '%c' in %s:%i.\n", \
            c, var, lineref->file_names[lineref->file_index[get_line_number(fp)]], \
            lineref->line_number[get_line_number(fp)]); \
        return 5; \
    }


int rapify_token(FILE *f_source, FILE *f_target, char *name, struct lineref *lineref) {
    int i;
    int line;
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
        line = warning_muted("unquoted-string") ? 0 : get_line_number(f_source);
        if (name == NULL)
            nwarningf("unquoted-string", "String in array is not quoted properly in %s:%i.\n", lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
        else
            nwarningf("unquoted-string", "String \"%s\" is not quoted properly in %s:%i.\n", name, lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
    }

    fwrite(buffer, strlen(buffer) + 1, 1, f_target);

    return 0;
}


int rapify_array(FILE *f_source, FILE *f_target, struct lineref *lineref) {
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
            success = rapify_array(f_source, f_target, lineref);
            if (success)
                return success;
        } else {
            success = rapify_token(f_source, f_target, NULL, lineref);
            if (success)
                return success;
        }
    }

    return 0;
}


int skip_to_semicolon(FILE *f_source) {
    /*
     * Skip ahead until after the closing ; after a class definition.
     *
     * Return 0 on success and a positive integer on failure.
     */

    int level;
    char in_string;
    char c;

    in_string = 0;
    level = 0;

    while (true) {
        if (feof(f_source))
            return 1;

        c = fgetc(f_source);

        if (in_string) {
            if (c == in_string)
                in_string = 0;
            continue;
        }
        if (c == '"' || c == '\'') {
            in_string = c;
            continue;
        }

        if (c == '{')
            level++;
        else if (c == '}')
            level--;

        if (c == ';' && level == 0)
            return 0;
    }
}


bool check_name(char *name) {
    int i;

    if (strlen(name) == 0)
        return false;

    for (i = 0; i < strlen(name); i++) {
        if (name[i] >= 'a' && name[i] <= 'z')
            continue;
        if (name[i] >= 'A' && name[i] <= 'Z')
            continue;
        if (name[i] >= '0' && name[i] <= '9')
            continue;
        if (name[i] == '_')
            continue;

        return false;
    }

    return true;
}



int rapify_class(FILE *f_source, FILE *f_target, struct lineref *lineref, int level) {
    /*
     * Rapifies the current class in f_source and writes to f_target. Level
     * should be 0 initially. When rapifying the root class, the source pointer
     * should point to the start of the file, if not, it should be just after
     * the classname and any whitespace following it, like so:
     *
     * class myclass : parentclass {
     *               ^
     *
     * Returns 0 on success and a positive integer on failure.
     */

    int success;
    int line;
    long i;
    long fp_end;
    long fp_source_classes[MAXCLASSES];
    long fp_target_classes[MAXCLASSES];
    uint8_t type;
    uint32_t num_entries;
    uint32_t num_classes;
    uint32_t fp_temp;
    char word[1024];
    char c;

    if (level == 0) {
        fputc(0, f_target); // inherited classname
    } else {
        GET_C(f_source, c);
        if (c == ':') {
            SKIP_WHITESPACE(f_source);
            GET_WORD(f_source, word);

            if (!check_name(word)) {
                line = get_line_number(f_source);
                errorf("Invalid characters in parent class name \"%s\" in %s:%i.\n", word,
                        lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
                return 3;
            }
            fwrite(word, strlen(word) + 1, 1, f_target);

            SKIP_WHITESPACE(f_source);
            GET_C(f_source, c);
        } else {
            fputc(0, f_target);
        }

        EXPECT_C(f_source, c, '{', lineref);
    }

    fp_temp = ftell(f_source);
    num_entries = 0;
    uint32_t fp_temp_2;
    while (true) {
        if (skip_whitespace(f_source))
            break;
        if (lookahead_c(f_source) == '}')
            break;
        num_entries++;
        
        if (false) {
            fp_temp_2 = ftell(f_source);
            fgets(word, sizeof(word), f_source);
            fseek(f_source, fp_temp_2, SEEK_SET);
        }

        if (skip_to_semicolon(f_source))
            break;
    }
    write_compressed_int(num_entries, f_target);
    fseek(f_source, fp_temp, SEEK_SET);

    num_classes = 0;
    for (i = 0; i < num_entries; i++) {
        SKIP_WHITESPACE(f_source);

        LOOKAHEAD_C(f_source, c);
        GET_WORD(f_source, word);
        if (c == '}' || strlen(word) == 0) {
            errorf("Less entries in class than expected (usually caused by wrong {}s or missing ;s).\n");
            return 1;
        }

        if (strcmp(word, "class") == 0)
            type = 0;
        else if (strcmp(word, "delete") == 0)
            type = 4;
        else
            type = 1;

        // class
        if (type == 0 || type == 4) {
            GET_C(f_source, c);
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                line = get_line_number(f_source);
                errorf("Expected space after %s keyword, got '%c'.\n", word, c,
                        lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
                return 9;
            }

            SKIP_WHITESPACE(f_source);
            GET_WORD(f_source, word);

            if (!check_name(word)) {
                line = get_line_number(f_source);
                errorf("Invalid characters in class name \"%s\" in %s:%i.\n", word,
                        lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
                return 12;
            }

            SKIP_WHITESPACE(f_source);
            LOOKAHEAD_C(f_source, c);

            // external class
            if (c == ';') {
                type = (type == 0) ? 3 : 4;

                fseek(f_source, 1, SEEK_CUR);

                fputc((char)type, f_target);
                fwrite(word, strlen(word) + 1, 1, f_target);

                continue;
            } else if (type == 4) {
                line = get_line_number(f_source);
                errorf("Expected ';' after delete statement, got: '%c' in %s:%i.\n", c,
                        lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
                return 15;
            }

            if (num_classes >= MAXCLASSES) {
                errorf("MAXCLASSES exceeded.\n");
                return 16;
            }

            fp_source_classes[num_classes] = ftell(f_source);

            fputc((char)type, f_target);
            fp_target_classes[num_classes] = ftell(f_target);
            fwrite(word, strlen(word) + 1, 1, f_target);
            fwrite("\0\0\0\0", 4, 1, f_target);

            if (skip_to_semicolon(f_source)) {
                fseek(f_source, fp_source_classes[num_classes], SEEK_SET);
                line = get_line_number(f_source);
                errorf("Failed to skip to EOC for class \"%s\" in %s:%i.\n", word,
                        lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
                return 17;
            }

            num_classes++;
            continue;
        }
        
        // variable or array
        if (!check_name(word)) {
            line = get_line_number(f_source);
            errorf("Invalid characters in variable name \"%s\" in %s:%i.\n", word,
                    lineref->file_names[lineref->file_index[line]], lineref->line_number[line]);
            return 18;
        }

        SKIP_WHITESPACE(f_source);
        LOOKAHEAD_C(f_source, c);
        
        // array
        if (c == '[') {
            type++;

            fseek(f_source, 1, SEEK_CUR);
            SKIP_WHITESPACE(f_source);
            GET_C(f_source, c);
            EXPECT_C(f_source, c, ']', lineref);
        }

        SKIP_WHITESPACE(f_source);
        GET_C(f_source, c);
        if (type == 2 && c == '+') {
            type = 5;
            GET_C(f_source, c);
        }

        EXPECT_C(f_source, c, '=', lineref);
        SKIP_WHITESPACE(f_source);

        fputc((char)type, f_target);

        if (type == 2 || type == 5) {
            fwrite(word, strlen(word) + 1, 1, f_target);

            success = rapify_array(f_source, f_target, lineref);

            SKIP_WHITESPACE(f_source);
            GET_C(f_source, c);
            EXPECT_C(f_source, c, ';', lineref);
        } else {
            success = rapify_token(f_source, f_target, word, lineref);
        }

        if (success) {
            errorf("Failed to rapify variable \"%s\".\n", word);
            return success;
        }
    }

    if (level > 0) {
        SKIP_WHITESPACE(f_source);
        GET_C(f_source, c);
        EXPECT_C(f_source, c, '}', lineref);
        SKIP_WHITESPACE(f_source);
        GET_C(f_source, c);
        EXPECT_C(f_source, c, ';', lineref);
    }

    fp_end = ftell(f_source);

    for (i = 0; i < num_classes; i++) {
        fseek(f_source, fp_source_classes[i], SEEK_SET);

        fp_temp = ftell(f_target);
        fseek(f_target, fp_target_classes[i], SEEK_SET);

        GET_WORD(f_target, word);

        fseek(f_target, 1, SEEK_CUR);
        fwrite(&fp_temp, sizeof(uint32_t), 1, f_target);
        fseek(f_target, 0, SEEK_END);

        success = rapify_class(f_source, f_target, lineref, level + 1);
        if (success) {
            errorf("Failed to rapify class \"%s\".\n", word);
            return success;
        }
    }

    fseek(f_source, fp_end, SEEK_SET);

    return 0;
}


int rapify_file(char *source, char *target) {
    /*
     * Resolves macros/includes and rapifies the given file. If source and
     * target are identical, the target is overwritten.
     *
     * Returns 0 on success and a positive integer on failure.
     */

    extern char include_stack[MAXINCLUDES][1024];
    FILE *f_temp;
    FILE *f_target;
    int i;
    int datasize;
    int success;
    char dump_name[2048];
    char buffer[4096];
    uint32_t enum_offset = 0;
    struct constant *constants;
    struct lineref *lineref;

    current_operation = OP_RAPIFY;
    strcpy(current_target, source);

    // Check if the file is already rapified
    f_temp = fopen(source, "rb");
    if (!f_temp) {
        errorf("Failed to open %s.\n", source);
        return 1;
    }

    fread(buffer, 4, 1, f_temp);
    if (strncmp(buffer, "\0raP", 4) == 0) {
        f_target = fopen(target, "wb");
        if (!f_target) {
            errorf("Failed to open %s.\n", target);
            fclose(f_temp);
            return 2;
        }

        fseek(f_temp, 0, SEEK_END);
        datasize = ftell(f_temp);

        fseek(f_temp, 0, SEEK_SET);
        for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
            fread(buffer, sizeof(buffer), 1, f_temp);
            fwrite(buffer, sizeof(buffer), 1, f_target);
        }
        fread(buffer, datasize - i, 1, f_temp);
        fwrite(buffer, datasize - i, 1, f_target);

        fclose(f_temp);
        fclose(f_target);

        return 0;
    } else {
        fclose(f_temp);
    }

#ifdef _WIN32
    char temp_name[2048];
    if (!GetTempFileName(".", "amk", 0, temp_name)) {
        errorf("Failed to get temp file name (system error %i).\n", GetLastError());
        return 1;
    }
    f_temp = fopen(temp_name, "wb+");
#else
    f_temp = tmpfile();
#endif

    if (!f_temp) {
        errorf("Failed to open temp file.\n");
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return 1;
    }

    // Write original file to temp and pre process
    constants = (struct constant *)malloc(MAXCONSTS * sizeof(struct constant));
    for (i = 0; i < MAXCONSTS; i++) {
        constants[i].name[0] = 0;
        constants[i].arguments[0][0] = 0;
        constants[i].value = 0;
    }

    lineref = (struct lineref *)malloc(sizeof(struct lineref));
    lineref->num_files = 0;
    lineref->num_lines = 0;
    lineref->file_index = (uint32_t *)malloc(sizeof(uint32_t) * LINEINTERVAL);
    lineref->line_number = (uint32_t *)malloc(sizeof(uint32_t) * LINEINTERVAL);

    for (i = 0; i < MAXINCLUDES; i++)
        include_stack[i][0] = 0;

    success = preprocess(source, f_temp, constants, lineref);

    for (i = 0; i < MAXCONSTS && constants[i].value != 0; i++)
        free(constants[i].value);
    free(constants);

    current_operation = OP_RAPIFY;
    strcpy(current_target, source);

    if (success) {
        errorf("Failed to preprocess %s.\n", source);
        fclose(f_temp);
#ifdef _WIN32
        DeleteFile(temp_name);
#endif
        return success;
    }

#if 0
    FILE *f_dump;

    sprintf(dump_name, "armake_preprocessed_%u.dump", (unsigned)time(NULL));
    printf("Done with preprocessing, dumping preprocessed config to %s.\n", dump_name);

    f_dump = fopen(dump_name, "wb");
    fseek(f_temp, 0, SEEK_END);
    datasize = ftell(f_temp);

    fseek(f_temp, 0, SEEK_SET);
    for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
        fread(buffer, sizeof(buffer), 1, f_temp);
        fwrite(buffer, sizeof(buffer), 1, f_dump);
    }

    fread(buffer, datasize - i, 1, f_temp);
    fwrite(buffer, datasize - i, 1, f_dump);

    fclose(f_dump);
#endif

    // Rapify file
    fseek(f_temp, 0, SEEK_SET);
    f_target = fopen(target, "wb+");
    if (!f_target) {
        errorf("Failed to open %s.\n", target);
        fclose(f_temp);
        return 2;
    }
    fwrite("\0raP", 4, 1, f_target);
    fwrite("\0\0\0\0\x08\0\0\0", 8, 1, f_target);
    fwrite(&enum_offset, 4, 1, f_target); // this is replaced later

    success = rapify_class(f_temp, f_target, lineref, 0);
    if (success) {
        sprintf(dump_name, "armake_preprocessed_%u.dump", (unsigned)time(NULL));
        errorf("Failed to rapify %s,\n       dumping preprocessed config to %s.\n", source, dump_name);

        fclose(f_target);

        f_target = fopen(dump_name, "wb");

        fseek(f_temp, 0, SEEK_END);
        datasize = ftell(f_temp);

        fseek(f_temp, 0, SEEK_SET);
        for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
            fread(buffer, sizeof(buffer), 1, f_temp);
            fwrite(buffer, sizeof(buffer), 1, f_target);
        }
        fread(buffer, datasize - i, 1, f_temp);
        fwrite(buffer, datasize - i, 1, f_target);

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

    free(lineref->file_index);
    free(lineref->line_number);
    free(lineref);

    return 0;
}
