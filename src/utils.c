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
#include <math.h>

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"


bool float_equal(float f1, float f2, float precision) {
    /*
     * Performs a fuzzy float comparison.
     */

    return fabs(1.0 - (f1 / f2)) < precision;
}


void lower_case(char *string) {
    /*
     * Converts a null-terminated string to lower case.
     */

    int i;

    for (i = 0; i < strlen(string); i++) {
        if (string[i] >= 'A' && string[i] <= 'Z')
            string[i] -= 'A' - 'a';
    }
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


void trim_leading(char *string, size_t buffsize) {
    /*
     * Trims leading tabs and spaces on the string.
     */

    char tmp[buffsize];
    char *ptr = tmp;
    strncpy(tmp, string, buffsize - 2);
    while (*ptr == ' ' || *ptr == '\t')
        ptr++;
    strncpy(string, ptr, buffsize - (2 + ptr - tmp));
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


void write_compressed_int(uint32_t integer, FILE *f) {
    uint64_t temp;
    char c;

    temp = (uint64_t)integer;

    if (temp == 0) {
        fwrite(&temp, 1, 1, f);
    }

    while (temp > 0) {
        if (temp > 0x7f) {
            // there are going to be more entries
            c = 0x80 | (temp & 0x7f);
            fwrite(&c, 1, 1, f);
            temp = temp >> 7;
        } else {
            // last entry
            c = temp;
            fwrite(&c, 1, 1, f);
            temp = 0;
        }
    }
}


uint32_t read_compressed_int(FILE *f) {
    int i;
    uint64_t result;
    uint8_t temp;

    result = 0;

    for (i = 0; i <= 4; i++) {
        temp = fgetc(f);
        result = result | ((temp & 0x7f) << (i * 7));

        if (temp < 0x80)
            break;
    }

    return (uint32_t)result;
}
