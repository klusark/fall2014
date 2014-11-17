#pragma once

#include <vector>
#include <string>
#include "vector.h"
#include "sphere.h"

struct Face {
	int x;
	int y;
	int z;
	Vector norm;
};

class Model : public Object {
public:
	Model(const std::string &filename);
	float intersect(const Point &ray, const Vector &o, IntersectionInfo &out) const;
	Vector getNormal(const IntersectionInfo &) const;
private:
	std::vector<Vector> _vertices;
	std::vector<Face> _faces;
	Vector bbtop;
	Vector bbbottom;
};
