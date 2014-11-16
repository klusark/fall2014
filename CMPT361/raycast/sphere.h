#pragma once

/**********************************************************************
 * Some stuff to handle spheres
 **********************************************************************/
#include "vector.h"

class IntersectionInfo {
public:
	IntersectionInfo() : vertex(0) {}
	Point pos;
	int vertex;
};

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

	virtual float intersect(const Point &, const Vector &, IntersectionInfo &) = 0;
	virtual Vector getNormal(const IntersectionInfo &) = 0;
};

class Sphere : public Object {
public:
	Sphere(Point, float, float [], float [], float [], float, float, int);
	virtual float intersect(const Point &, const Vector &, IntersectionInfo &);
	virtual Vector getNormal(const IntersectionInfo &);

	int index;
	Point center;
	float radius;
};

