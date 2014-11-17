#pragma once

#include "sphere.h"


class Plane : public Object {
public:
	Plane(float [], float [], float [], float, float);
	virtual float intersect(const Point &, const Vector &, IntersectionInfo &) const;
	virtual Vector getNormal(const IntersectionInfo &) const;

	Vector normal;
};

