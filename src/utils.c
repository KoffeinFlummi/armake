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
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"


void infof(char *format, ...) {
    char buffer[4096];
    va_list argptr;

    va_start(argptr, format);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

#ifdef _WIN32
    fprintf(stdout, "info: %s", buffer);
#else
    fprintf(stdout, "%sinfo:%s %s", COLOR_GREEN, COLOR_RESET, buffer);
#endif

    fflush(stdout);
}


void debugf(char *format, ...) {
    char buffer[4096];
    va_list argptr;

    va_start(argptr, format);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

#ifdef _WIN32
    fprintf(stdout, "debug: %s", buffer);
#else
    fprintf(stdout, "%sdebug:%s %s", COLOR_CYAN, COLOR_RESET, buffer);
#endif

    fflush(stdout);
}


void warningf(char *format, ...) {
    extern int current_operation;
    extern char current_target[2048];
    char filename[2048];
    char buffer[4096];
    va_list argptr;

    va_start(argptr, format);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

#ifdef _WIN32
    fprintf(stderr, "warning: %s", buffer);
#else
    fprintf(stderr, "%swarning:%s %s", COLOR_YELLOW, COLOR_RESET, buffer);
#endif

    if (strchr(current_target, PATHSEP) == NULL)
        strcpy(filename, current_target);
    else
        strcpy(filename, strrchr(current_target, PATHSEP) + 1);

    if (current_operation == OP_BUILD)
        fprintf(stderr, "    (encountered while building %s)\n", filename);
    else if (current_operation == OP_PREPROCESS)
        fprintf(stderr, "    (encountered while preprocessing %s)\n", filename);
    else if (current_operation == OP_RAPIFY)
        fprintf(stderr, "    (encountered while rapifying %s)\n", filename);
    else if (current_operation == OP_P3D)
        fprintf(stderr, "    (encountered while converting %s)\n", filename);
    else if (current_operation == OP_MODELCONFIG)
        fprintf(stderr, "    (encountered while reading model config for %s)\n", filename);
    else if (current_operation == OP_MATERIAL)
        fprintf(stderr, "    (encountered while reading %s)\n", filename);
    else if (current_operation == OP_UNPACK)
        fprintf(stderr, "    (encountered while unpacking %s)\n", filename);
    else if (current_operation == OP_DERAPIFY)
        fprintf(stderr, "    (encountered while derapifying %s)\n", filename);

    fflush(stderr);
}


bool warning_muted(char *name) {
    extern char muted_warnings[MAXWARNINGS][512];
    int i;

    for (i = 0; i < MAXWARNINGS; i++) {
        if (strcmp(muted_warnings[i], name) == 0)
            return true;
    }
    return false;
}


void nwarningf(char *name, char *format, ...) {
    char buffer[4096];
    char temp[4096];
    va_list argptr;

    if (warning_muted(name))
        return;

    va_start(argptr, format);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

    if (buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = 0;

    sprintf(temp, "%s [%s]\n", buffer, name);

    warningf(temp);
}


void errorf(char *format, ...) {
    extern int current_operation;
    extern char current_target[2048];
    char buffer[4096];
    va_list argptr;

    va_start(argptr, format);
    vsprintf(buffer, format, argptr);
    va_end(argptr);

#ifdef _WIN32
    fprintf(stderr, "error: %s", buffer);
#else
    fprintf(stderr, "%serror:%s %s", COLOR_RED, COLOR_RESET, buffer);
#endif

    fflush(stderr);
}


int get_line_number(FILE *f_source) {
    int line;
    long fp_start;

    fp_start = ftell(f_source);
    fseek(f_source, 0, SEEK_SET);

    line = 0;
    while (ftell(f_source) < fp_start) {
        if (fgetc(f_source) == '\n')
            line++;
    }

    return line;
}


void reverse_endianness(void *ptr, size_t buffsize) {
    char *buffer;
    char *temp;
    int i;

    buffer = (char *)ptr;
    temp = malloc(buffsize);

    for (i = 0; i < buffsize; i++) {
        temp[(buffsize - 1) - i] = buffer[i];
    }

    memcpy(buffer, temp, buffsize);

    free(temp);
}


bool matches_glob(char *string, char *pattern) {
    char *ptr1 = string;
    char *ptr2 = pattern;

    while (*ptr1 != 0 && *ptr2 != 0) {
        if (*ptr2 == '*') {
            ptr2++;
            while (true) {
                if (matches_glob(ptr1, ptr2))
                    return true;
                if (*ptr1 == 0)
                    return false;
                ptr1++;
            }
        }

        if (*ptr2 != '?' && *ptr1 != *ptr2)
            return false;

        ptr1++;
        ptr2++;
    }

    return (*ptr1 == *ptr2);
}


#ifndef _WIN32
int stricmp(char *a, char *b) {
    int d;
    char a_lower;
    char b_lower;

    for (;; a++, b++) {
        a_lower = *a;
        b_lower = *b;

        if (a_lower >= 'A' && a_lower <= 'Z')
            a_lower -= 'A' - 'a';

        if (b_lower >= 'A' && b_lower <= 'Z')
            b_lower -= 'A' - 'a';

        d = a_lower - b_lower;
        if (d != 0 || !*a)
            return d;
    }
}
#endif


bool float_equal(float f1, float f2, float precision) {
    /*
     * Performs a fuzzy float comparison.
     */

    return fabs(1.0 - (f1 / f2)) < precision;
}


int fsign(float f) {
    return (0 < f) - (f < 0);
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


void trim_leading(char *string, size_t buffsize) {
    /*
     * Trims leading tabs and spaces on the string.
     */

    char tmp[buffsize];
    char *ptr = tmp;
    strncpy(tmp, string, buffsize);
    while (*ptr == ' ' || *ptr == '\t')
        ptr++;
    strncpy(string, ptr, buffsize - (ptr - tmp));
}


void trim(char *string, size_t buffsize) {
    /*
     * Trims tabs and spaces on either side of the string.
     */

    char *ptr;

    trim_leading(string, buffsize);

    ptr = string + (strlen(string) - 1);
    while (ptr >= string && (*ptr == ' ' || *ptr == '\t'))
        ptr--;

    *(ptr + 1) = 0;
}


void replace_string(char *string, size_t buffsize, char *search, char *replace, int max, bool macro) {
    /*
     * Replaces the search string with the given replacement in string.
     * max is the maximum number of occurrences to be replaced. 0 means
     * unlimited.
     */

    if (strstr(string, search) == NULL)
        return;

    char *tmp;
    char *ptr_old;
    char *ptr_next;
    char *ptr_tmp;
    int i;
    bool quote;

    tmp = malloc(buffsize);
    strncpy(tmp, string, buffsize);

    ptr_old = string;
    ptr_tmp = tmp;

    for (i = 0;; i++) {
        ptr_next = strstr(ptr_old, search);
        ptr_tmp += (ptr_next - ptr_old);
        if (ptr_next == NULL || (i >= max && max != 0))
            break;

        quote = false;
        if (macro && ptr_next > string + 1 && *(ptr_next - 2) == '#' && *(ptr_next - 1) == '#')
            ptr_next -= 2;
        else if ((quote = macro && (ptr_next > string && *(ptr_next - 1) == '#')))
            *(ptr_next - 1) = '"';

        strncpy(ptr_next, replace, buffsize - (ptr_next - string));
        ptr_next += strlen(replace);

        if (quote)
            *(ptr_next++) = '"';

        ptr_tmp += strlen(search);

        if (macro && *ptr_tmp == '#' && *(ptr_tmp + 1) == '#')
            ptr_tmp += 2;

        strncpy(ptr_next, ptr_tmp, buffsize - (ptr_next - string));

        ptr_old = ptr_next;
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
     * Returns a char on success, 1 on failure.
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
            break;
        if (buffer[i] <= ' ' || buffer[i] == '\t' || buffer[i] == '\r' || buffer[i] == '\n' ||
                buffer[i] == ',' || buffer[i] == ';' || buffer[i] == '{' ||
                buffer[i] == '}' || buffer[i] == '(' || buffer[i] == ')' ||
                buffer[i] == '=' || buffer[i] == '[' || buffer[i] == ']' ||
                buffer[i] == ':') {
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
        if (feof(f))
            return 1;
    } while (current == ' ' || current == '\t' || current == '\r' || current == '\n');

    fseek(f, -1, SEEK_CUR);
    return 0;
}


void escape_string(char *buffer, size_t buffsize) {
    char *tmp;
    char *ptr;
    char tmp_array[3];

    tmp = malloc(buffsize * 2);
    tmp[0] = 0;
    tmp_array[2] = 0;

    for (ptr = buffer; *ptr != 0; ptr++) {
        tmp_array[0] = '\\';
        if (*ptr == '\r') {
            tmp_array[1] = 'r';
        } else if (*ptr == '\n') {
            tmp_array[1] = 'n';
        } else if (*ptr == '"') {
            tmp_array[0] = '"';
            tmp_array[1] = '"';
        } else {
            tmp_array[0] = *ptr;
            tmp_array[1] = 0;
        }
        strcat(tmp, tmp_array);
    }

    strncpy(buffer, tmp, buffsize);

    free(tmp);
}


void unescape_string(char *buffer, size_t buffsize) {
    char *tmp;
    char *ptr;
    char tmp_array[2];
    char current;
    char quote;

    tmp = malloc(buffsize);
    tmp[0] = 0;
    tmp_array[1] = 0;

    quote = buffer[0];

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
        } else if (*ptr == quote && *(ptr + 1) == quote) {
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
