#include "matrix.h"

matrix vectorTilda(const vector v) {
    matrix r = EmptyMatrix;

    r.m01 = -v.z;
    r.m02 =  v.y;
    r.m10 =  v.z;
    r.m12 = -v.x;
    r.m20 = -v.y;
    r.m21 =  v.x;
    
    return r;
}

matrix matrixSub(const matrix m1, const matrix m2) {
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

matrix matrixMult(const matrix a, const matrix b) {
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

matrix matrixMultScalar(const float s, const matrix m) {
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

matrix matrixInverse(const matrix m)
{
    matrix r;
    float determinant = 0;

    determinant += m.m10 * ( m.m21 * m.m02 - m.m01 * m.m22);
    determinant += m.m20 * ( m.m01 * m.m12 - m.m11 * m.m02);

    r.m00 = ((m.m11 * m.m22) - (m.m21 * m.m12)) / determinant;
    r.m10 = ((m.m21 * m.m02) - (m.m01 * m.m22)) / determinant;
    r.m20 = ((m.m01 * m.m12) - (m.m11 * m.m02)) / determinant;
    r.m01 = ((m.m12 * m.m20) - (m.m22 * m.m10)) / determinant;
    r.m11 = ((m.m22 * m.m00) - (m.m02 * m.m20)) / determinant;
    r.m21 = ((m.m02 * m.m10) - (m.m12 * m.m00)) / determinant;
    r.m02 = ((m.m10 * m.m21) - (m.m20 * m.m11)) / determinant;
    r.m12 = ((m.m20 * m.m01) - (m.m00 * m.m21)) / determinant;
    r.m22 = ((m.m00 * m.m11) - (m.m10 * m.m01)) / determinant;

   return r;
}
