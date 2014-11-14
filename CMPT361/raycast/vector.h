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
Vector vec_cross(const Vector &a, const Vector &b);
Vector vec_reflect(const Vector &, const Vector &);
Vector vec_refract(const Vector &, const Vector &);
Vector get_vec(const Point &, const Point &);
Point get_point(const Point &, const Vector &);
void normalize(Vector *);

RGB_float operator +(const RGB_float &, const RGB_float &);
RGB_float operator *(const RGB_float &, float);
void operator +=(RGB_float &, const RGB_float &);
void operator /=(RGB_float &, float);

Vector operator *(const Vector &, float v);
Vector operator +(const Vector &, const Vector &);
Vector operator -(const Vector &, const Vector &);
