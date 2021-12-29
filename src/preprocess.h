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
#define FILEINTERVAL 32
#define LINEINTERVAL 1024


struct constant {
    char *name;
    char *value;
    int num_args;
    int num_occurences;
    int (*occurrences)[2];
    struct constant *next;
    struct constant *last;
};

struct constants {
    struct constant *head;
    struct constant *tail;
};

struct lineref {
    uint32_t num_files;
    uint32_t num_lines;
    char **file_names;
    uint32_t *file_index;
    uint32_t *line_number;
};

struct constant_stack {
    struct constant *constant;
    struct constant_stack *next;
};


extern char include_stack[MAXINCLUDES][1024];


struct constants *constants_init();
bool constants_parse(struct constants *constants, char *definition, int line);
bool constants_remove(struct constants *constants, char *name);
struct constant *constants_find(struct constants *constants, char *name, int len);
char *constants_preprocess(struct constants *constants, char *source, int line, struct constant_stack *constant_stack);
void constants_free(struct constants *constants);

char *constant_value(struct constants *constants, struct constant *constant,
        int num_args, char **args, int value, struct constant_stack *constant_stack);
void constant_free(struct constant *constant);

bool matches_includepath(char *path, char *includepath, char *includefolder);

int find_file(char *includepath, char *origin, char *actualpath);

char * resolve_macros(char *string, size_t buffsize, struct constant *constants);

int preprocess(char *source, FILE *f_target, struct constants *constants, struct lineref *lineref);
