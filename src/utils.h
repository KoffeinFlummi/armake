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

#pragma once


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define COLOR_RED "\e[1;31m"
#define COLOR_GREEN "\e[1;32m"
#define COLOR_YELLOW "\e[1;33m"
#define COLOR_BLUE "\e[1;34m"
#define COLOR_MAGENTA "\e[1;35m"
#define COLOR_CYAN "\e[1;36m"
#define COLOR_RESET "\e[0m"

#define OP_BUILD 1
#define OP_PREPROCESS 2
#define OP_RAPIFY 3
#define OP_P3D 4
#define OP_MODELCONFIG 5
#define OP_MATERIAL 6
#define OP_UNPACK 7
#define OP_DERAPIFY 8


#include "docopt.h"


struct point {
    float x;
    float y;
    float z;
    uint32_t point_flags;
};

int current_operation;
char current_target[2048];


void warningf(char *format, ...);

void nwarningf(char *name, char *format, ...);

void errorf(char *format, ...);

bool matches_glob(char *string, char *pattern);

#ifndef _WIN32
int stricmp(char *a, char *b);
#endif

bool float_equal(float f1, float f2, float precision);

void lower_case(char *string);

void get_word(char *target, char *source);

void trim_leading(char *string, size_t buffsize);

void trim(char *string, size_t buffsize);

void replace_string(char *string, size_t buffsize, char *search, char *replace, int max);

void quote(char *string);

char lookahead_c(FILE *f);

int lookahead_word(FILE *f, char *buffer, size_t buffsize);

int skip_whitespace(FILE *f);

void escape_string(char *buffer, size_t buffsize);

void unescape_string(char *buffer, size_t buffsize);

void write_compressed_int(uint32_t integer, FILE *f);

uint32_t read_compressed_int(FILE *f);
