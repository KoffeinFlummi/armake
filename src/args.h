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


#include <stdbool.h>


struct arguments {
    int num_positionals;
    char **positionals;
    bool help;
    bool version;
    bool force;
    bool packonly;
    bool compress;
    char *privatekey;
    char *signature;
    char *indent;
    char *paatype;
    int num_mutedwarnings;
    char **mutedwarnings;
    int num_includefolders;
    char **includefolders;
    int num_excludefiles;
    char **excludefiles;
    int num_headerextensions;
    char **headerextensions;
} args;

struct arg_option {
    char *short_name;
    char *long_name;
    void *value;
    int *length;
};
