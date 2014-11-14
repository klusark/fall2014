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
	int intersect(const Vector &ray, const Vector &o, Point &out);
	Vector getNormal(int);
private:
	std::vector<Vector> _vertices;
	std::vector<Face> _faces;
};
