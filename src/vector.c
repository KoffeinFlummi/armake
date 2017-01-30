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


#include <math.h>

#include "vector.h"


vector vector_add(const vector v1, const vector v2) {
    vector v;
    
    v.x = v1.x + v2.x;
    v.y = v1.y + v2.y;
    v.z = v1.z + v2.z;
    
    return v;
}

vector vector_sub(const vector v1, const vector v2) {
    vector v;
    
    v.x = v1.x - v2.x;
    v.y = v1.y - v2.y;
    v.z = v1.z - v2.z;
    
    return v;
}

vector vector_mult_scalar(const float s, const vector v) {
    vector r;
    
    r.x = v.x * s;
    r.y = v.y * s;
    r.z = v.z * s;
    
    return r;
}

vector vector_normalize(const vector v) {
    vector r;
    float magnitude;

    magnitude = vector_length(v);

    r.x = v.x / magnitude;
    r.y = v.y / magnitude;
    r.z = v.z / magnitude;

    return r;
}

vector vector_crossproduct(const vector v1, const vector v2) {
    vector r;

    r.x = v1.y * v2.z - v1.z * v2.y;
    r.y = v1.z * v2.x - v1.x * v2.z;
    r.z = v1.x * v2.y - v1.y * v2.x;

    return r;
}

float vector_length(const vector v) {
    return (float)sqrt((double)(v.x * v.x + v.y * v.y + v.z * v.z));
}
