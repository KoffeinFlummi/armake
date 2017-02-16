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


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


#define MAXCONSTS 4096
#define MAXARGS 32
#define MAXINCLUDES 64
#define LINEBUFFSIZE 131072
#define LINEINTERVAL 1024


struct constant {
    char name[256];
    char arguments[MAXARGS][512];
    char *value;
};

struct lineref {
    uint32_t num_files;
    uint32_t num_lines;
    char file_names[MAXINCLUDES][1024];
    uint32_t *file_index;
    uint32_t *line_number;
};


char include_stack[MAXINCLUDES][1024];


bool matches_includepath(char *path, char *includepath, char *includefolder);

int find_file(char *includepath, char *origin, char *actualpath);

int resolve_macros(char *string, size_t buffsize, struct constant *constants);

int preprocess(char *source, FILE *f_target, struct constant *constants, struct lineref *lineref);
