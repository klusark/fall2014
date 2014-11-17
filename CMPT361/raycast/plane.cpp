#include "plane.h"
#include <stdlib.h>
#include <math.h>
#include <cstdio>
#include <algorithm>

float Plane::intersect(const Point &pos, const Vector &ray, IntersectionInfo &hit) {
	Vector n = normal;
	float denom = vec_dot(n, ray);
	if (fabs(denom) > 0.001) {
		Vector p2 = {0,-3,0};
		Vector p = {pos.x, pos.y, pos.z};
		Vector p0 = p2 - p;
		float t = vec_dot(p0, n) / denom;
		if (t >= 0) {
			Vector sc = ray * t;
			hit.pos = get_point(pos, sc);
			hit.vertex = 5555;
			return t;
		}
	}
	return -1;
}


Plane::Plane(float amb[],
				float dif[], float spe[], float shine,
				float refl) : normal({0,1,0}) {
	normalize(&normal);
	mat_ambient[0] = amb[0];
	mat_ambient[1] = amb[1];
	mat_ambient[2] = amb[2];
	mat_diffuse[0] = dif[0];
	mat_diffuse[1] = dif[1];
	mat_diffuse[2] = dif[2];
	mat_specular[0] = spe[0];
	mat_specular[1] = spe[1];
	mat_specular[2] = spe[2];
	mat_shineness = shine;
	reflectance = refl;
}

/******************************************
 * computes a sphere normal - done for you
 ******************************************/
Vector Plane::getNormal(const IntersectionInfo &info) {
	return normal;
}
