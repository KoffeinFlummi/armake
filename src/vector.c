#include "vector.h"

vector vectorAdd(const vector v1, const vector v2) {
	vector v;
	
	v.x = v1.x + v2.x;
	v.y = v1.y + v2.y;
	v.z = v1.z + v2.z;
	
	return v;
}

vector vectorSub(const vector v1, const vector v2) {
	vector v;
	
	v.x = v1.x - v2.x;
	v.y = v1.y - v2.y;
	v.z = v1.z - v2.z;
	
	return v;
}

vector vectorMultScalar(const float s, const vector v) {
	vector r;
	
	r.x = v.x * s;
	r.y = v.y * s;
	r.z = v.z * s;
	
	return r;
}