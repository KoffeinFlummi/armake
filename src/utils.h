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

#ifndef __utils_h
#define __utils_h

#include "docopt.h"


void get_word(char *target, char *source);

void trim_leading(char *string, size_t buffsize);

void replace_string(char *string, size_t buffsize, char *search, char *replace, int max);

void quote(char *string);

char lookahead_c(FILE *f);

int lookahead_word(FILE *f, char *buffer, size_t buffsize);

int skip_whitespace(FILE *f);

void unescape_string(char *buffer, size_t buffsize);

void write_compressed_int(uint32_t integer, FILE *f_target);

#endif
