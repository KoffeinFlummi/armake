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
#include <string.h>

#include "matrix.h"


matrix vector_tilda(const vector v) {
    matrix r = empty_matrix;

    r.m01 = -v.z;
    r.m02 =  v.y;
    r.m10 =  v.z;
    r.m12 = -v.x;
    r.m20 = -v.y;
    r.m21 =  v.x;
    
    return r;
}

matrix matrix_sub(const matrix m1, const matrix m2) {
    matrix r;

    r.m00 = m1.m00 - m2.m00;
    r.m10 = m1.m10 - m2.m10;
    r.m20 = m1.m20 - m2.m20;
    r.m01 = m1.m01 - m2.m01;
    r.m11 = m1.m11 - m2.m11;
    r.m21 = m1.m21 - m2.m21;
    r.m02 = m1.m02 - m2.m02;
    r.m12 = m1.m12 - m2.m12;
    r.m22 = m1.m22 - m2.m22;
    
    return r;
}

matrix matrix_mult(const matrix a, const matrix b) {
    float x, y, z;
    matrix r;

    x = b.m00;
    y = b.m10;
    z = b.m20;
    r.m00 = a.m00 * x + a.m01 * y + a.m02 * z;
    r.m10 = a.m10 * x + a.m11 * y + a.m12 * z;
    r.m20 = a.m20 * x + a.m21 * y + a.m22 * z;

    x = b.m01;
    y = b.m11;
    z = b.m21;
    r.m01 = a.m00 * x + a.m01 * y + a.m02 * z;
    r.m11 = a.m10 * x + a.m11 * y + a.m12 * z;
    r.m21 = a.m20 * x + a.m21 * y + a.m22 * z;

    x = b.m02;
    y = b.m12;
    z = b.m22;
    r.m02 = a.m00 * x + a.m01 * y + a.m02 * z;
    r.m12 = a.m10 * x + a.m11 * y + a.m12 * z;
    r.m22 = a.m20 * x + a.m21 * y + a.m22 * z;

    return r;
}

matrix matrix_mult_scalar(const float s, const matrix m) {
    matrix r;
    
    r.m00 = m.m00 * s;
    r.m10 = m.m10 * s;
    r.m20 = m.m20 * s;
    r.m01 = m.m01 * s;
    r.m11 = m.m11 * s;
    r.m21 = m.m21 * s;
    r.m02 = m.m02 * s;
    r.m12 = m.m12 * s;
    r.m22 = m.m22 * s;

    return r;
}

float matrix_determinant(const matrix m) {
    return (m.m00 * m.m11 * m.m22) + (m.m01 * m.m12 * m.m20) + (m.m02 * m.m10 * m.m21) -
            (m.m20 * m.m11 * m.m02) - (m.m21 * m.m12 * m.m00) - (m.m22 * m.m10 * m.m01);
}

matrix matrix_adjoint(const matrix m) {
    matrix r;

    memcpy(&r, &m, sizeof(matrix));

    r.m01 = m.m10;
    r.m02 = m.m20;

    r.m10 = m.m01;
    r.m12 = m.m21;

    r.m20 = m.m02;
    r.m21 = m.m12;

    return r;
}

matrix matrix_inverse(const matrix m) {
    matrix r;

    r = matrix_mult_scalar(1.0f / matrix_determinant(m), matrix_adjoint(m));

    return r;
}
