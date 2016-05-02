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


typedef struct triplet {
    float x;
    float y;
    float z;
} vector;


static const vector empty_vector = {0, 0, 0};


vector vector_add(const vector v1, const vector v2);

vector vector_sub(const vector v1, const vector v2);

vector vector_mult_scalar(const float s, const vector v);

vector vector_normalize(const vector v);

vector vector_crossproduct(const vector v1, const vector v2);

float vector_length(const vector v);
