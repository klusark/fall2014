#pragma once

/**************************************************************************
 * This header provides you data types and functions to handle vectors
 ************************************************************************/
typedef struct {
	float x;
	float y;
	float z;
} Point; // geometric 3D point

typedef struct {
	float x;
	float y;
	float z;
} Vector; // geometric 3D vector

typedef struct {
	float r;
	float g;
	float b;
} RGB_float;

float vec_len(const Vector &);
float vec_dot(const Vector &, const Vector &);
Vector vec_reflect(Vector, Vector);
Vector get_vec(Point, Point);
Point get_point(Point, Vector);
void normalize(Vector *);
RGB_float clr_add(RGB_float, RGB_float);
RGB_float clr_scale(RGB_float, float);

Vector operator *(const Vector &, float v);
Vector operator -(const Vector &, const Vector &);
