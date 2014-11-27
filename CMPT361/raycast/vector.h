#pragma once

#include "include/mat.h"
#include "include/vec.h"

/**************************************************************************
 * This header provides you data types and functions to handle vectors
 ************************************************************************/

#define Vector vec3

typedef struct {
	float x;
	float y;
	float z;
} Point; // geometric 3D point


struct RGB_float{
	float r;
	float g;
	float b;
};

Vector vec_reflect(const Vector &, const Vector &);
Vector vec_refract(const Vector &, const Vector &, float n1, float n2);
Vector get_vec(const Point &, const Point &);
Point get_point(const Point &, const Vector &);

RGB_float operator +(const RGB_float &, const RGB_float &);
RGB_float operator *(const RGB_float &, float);
void operator +=(RGB_float &, const RGB_float &);
void operator *=(RGB_float &, float);
void operator /=(RGB_float &, float);

