/**************************************************************
 * functions to handle vectors and points
 **************************************************************/
#include "vector.h"
#include <math.h>

Vector vec_reflect(const Vector &v, const Vector &norm) {
	return v - (norm * 2 * dot(v, norm));
}

Vector vec_refract(const Vector &v, const Vector &norm) {
	float n = 1/1;
	float a = -dot(v, norm);
	float s = sqrt(1 - ((n * n) * (1 - (a * a))));
	return (v * n) + (norm * ((n * a) - s));
}


//
// return vector from point point to another
//
Vector get_vec(const Point &q, const Point &p) {
	Vector rc;
	rc.x = p.x - q.x;
	rc.y = p.y - q.y;
	rc.z = p.z - q.z;

	return rc;
}

//
// return point from a point and a vector
//
Point get_point(const Point &p, const Vector &q) {
	Point rc;
	rc.x = p.x + q.x;
	rc.y = p.y + q.y;
	rc.z = p.z + q.z;

	return rc;
}

//
// add two RGB colors
//
RGB_float operator +(const RGB_float &p, const RGB_float &q) {
	RGB_float ret;

	ret.r = p.r + q.r;
	ret.g = p.g + q.g;
	ret.b = p.b + q.b;

	return ret;
}

RGB_float operator *(const RGB_float &p, float s) {
	RGB_float ret;

	ret.r = p.r * s;
	ret.g = p.g * s;
	ret.b = p.b * s;

	return ret;
}

void operator /=(RGB_float &p, float s) {
	p.r /= s;
	p.g /= s;
	p.b /= s;
}

void operator +=(RGB_float &p, const RGB_float &q) {
	p.r += q.r;
	p.g += q.g;
	p.b += q.b;
}
