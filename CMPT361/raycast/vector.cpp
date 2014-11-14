/**************************************************************
 * functions to handle vectors and points
 **************************************************************/
#include "vector.h"
#include <math.h>

Vector vec_reflect(const Vector &v, const Vector &norm) {
	return v - (norm * 2 * vec_dot(v, norm));
}

Vector vec_refract(const Vector &v, const Vector &norm) {
	float n = 1/1;
	float c1 = -vec_dot(v, norm);
	float c2 = sqrt(1 - n*n * (1- c1*c1));
	return (v * n) + norm * (n * c1 - c2);
}

Vector operator *(const Vector &v, float s) {
	Vector ret;

	ret.x = s * v.x;
	ret.y = s * v.y;
	ret.z = s * v.z;

	return ret;
}

Vector operator -(const Vector &p, const Vector &q) {
	Vector rc;
	rc.x = p.x - q.x;
	rc.y = p.y - q.y;
	rc.z = p.z - q.z;

	return rc;
}
//
// return length of a vector
//
float vec_len(const Vector &u) {
	return sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
}

//
// return doc product of two vectors
//
float vec_dot(const Vector &p, const Vector &q) {
	return p.x * q.x + p.y * q.y + p.z * q.z;
}

Vector vec_cross(const Vector &a, const Vector &b )
{
	Vector v = { a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x };
	return v;
}


//
// return sum of two vectors
//
Vector operator +(const Vector &p, const Vector &q) {
	Vector rc;
	rc.x = p.x + q.x;
	rc.y = p.y + q.y;
	rc.z = p.z + q.z;

	return rc;
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
// normalize a vector
//
void normalize(Vector *u) {
	float p;

	p = vec_len(*u);
	(*u).x = (*u).x / p;
	(*u).y = (*u).y / p;
	(*u).z = (*u).z / p;
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
