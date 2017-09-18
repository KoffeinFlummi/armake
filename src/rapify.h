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


#include "preprocess.h"


#define MAXCLASSES 4096


enum {
    TYPE_CLASS,
    TYPE_VAR,
    TYPE_ARRAY,
    TYPE_ARRAY_EXPANSION,
    TYPE_STRING,
    TYPE_INT,
    TYPE_FLOAT
};

struct definitions {
    struct definition *head;
};

struct definition {
    int type;
    void *content;
    struct definition *next;
};

struct class {
    char *name;
    char *parent;
    long offset_location;
    struct definitions *content;
};

struct variable {
    int type;
    char *name;
    struct expression *expression;
};

struct expression {
    int type;
    int32_t int_value;
    float float_value;
    char *string_value;
    struct expression *head;
    struct expression *next;
};


struct class *parse_file(FILE *f, struct lineref *lineref);

struct definitions *new_definitions();

struct definitions *add_definition(struct definitions *head, int type, void *content);

struct class *new_class(char *name, char *parent, struct definitions *content);

struct variable *new_variable(int type, char *name, struct expression *expression);

struct expression *new_expression(int type, void *value);

struct expression *add_expression(struct expression *head, struct expression *new);

void free_expression(struct expression *expr);

void free_variable(struct variable *var);

void free_definition(struct definition *definition);

void free_class(struct class *class);

void rapify_expression(struct expression *expr, FILE *f_target);

void rapify_variable(struct variable *var, FILE *f_target);

void rapify_class(struct class *class, FILE *f_target);

int rapify_file(char *source, char *target);
