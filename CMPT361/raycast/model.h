#pragma once

#include <vector>
#include <string>
#include "vector.h"

struct Face {
	int x;
	int y;
	int z;
};

class Model {
public:
	Model(const std::string &filename);
private:
	std::vector<Vector> _vertices;
	std::vector<Face> _faces;
};
