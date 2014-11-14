#pragma once

/**********************************************************************
 * Some stuff to handle spheres
 **********************************************************************/
#include "vector.h"


class Object {
public:

	float mat_ambient[3];	// material property used in Phong model
	float mat_diffuse[3];
	float mat_specular[3];
	float mat_shineness;

	float reflectance;		 // this number [0,1] determines how much
							 // reflected light contributes to the color
							 // of a pixel

	float transparency;
};

class Sphere : public Object {
public:
	Sphere(Point, float, float [], float [], float [], float, float, int);
	int index;
	Point center;
	float radius;
};

// intersect ray with sphere
float intersect_sphere(const Point &, const Vector &, Sphere *, Point &);
// return the unit normal at a point on sphere
Vector sphere_normal(const Point &, Sphere *);
// add a sphere to the sphere list

