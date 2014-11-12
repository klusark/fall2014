#pragma once

/**********************************************************************
 * Some stuff to handle spheres
 **********************************************************************/
#include "vector.h"

class Sphere {
public:
	Sphere(Point, float, float [], float [], float [], float, float, int);
	int index;				// identifies a sphere; must be greater than 0

	Point center;
	float radius;

	float mat_ambient[3];	// material property used in Phong model
	float mat_diffuse[3];
	float mat_specular[3];
	float mat_shineness;

	float reflectance;		 // this number [0,1] determines how much
							 // reflected light contributes to the color
							 // of a pixel
};

// intersect ray with sphere
float intersect_sphere(Point, Vector, Sphere *, Point *);
Sphere *intersect_scene(Point, Vector, Sphere *, Point *, int);
// return the unit normal at a point on sphere
Vector sphere_normal(Point, Sphere *);
// add a sphere to the sphere list

