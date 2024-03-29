#pragma once

#include "sphere.h"


class Plane : public Object {
public:
	Plane(float amb[], float dif[], float dif2[], float spe[], float, float,
	Vector, Vector, Vector, const Vector &p);
	virtual float intersect(const Point &, const Vector &, IntersectionInfo &) const;
	virtual Vector getNormal(const IntersectionInfo &) const;
	virtual float getDiffuse(const Point &, int) const;

private:
	Vector normal, _a, _b;
	Vector _pos;
	float mat_diffuse2[3];
};

