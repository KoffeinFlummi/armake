#pragma once

struct triplet {
    float x;
    float y;
    float z;
};

typedef struct triplet vector;

static const vector EmptyVector = {0, 0, 0};

vector vectorAdd(const vector v1, const vector v2);
vector vectorSub(const vector v1, const vector v2);
vector vectorMultScalar(const float s, const vector v);