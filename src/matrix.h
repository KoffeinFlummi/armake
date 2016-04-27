#pragma once

#include "vector.h"

typedef struct {
    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;
} matrix;

static const matrix EmptyMatrix = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const matrix IdentityMatrix = { 1, 0, 0, 0, 1, 0, 0, 0, 1 }; 

matrix vectorTilda(const vector v);

matrix matrixSub(const matrix m1, const matrix m2);
matrix matrixMult(const matrix m1, const matrix m2);
matrix matrixMultScalar(const float s, const matrix m);
matrix matrixInverse(const matrix m);
