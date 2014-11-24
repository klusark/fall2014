#include "plane.h"
#include <stdlib.h>
#include <math.h>
#include <cstdio>
#include <algorithm>

float Plane::intersect(const Point &pos, const Vector &ray, IntersectionInfo &hit) const {
	Vector n = normal;
	float denom = vec_dot(n, ray);
	if (fabs(denom) > 0.001) {
		Vector p2 = {0,-3,0};
		Vector p = {pos.x, pos.y, pos.z};
		Vector p0 = p2 - p;
		float t = vec_dot(p0, n) / denom;
		if (t >= 0.0001) {
			Vector sc = ray * t;
			hit.pos = get_point(pos, sc);
			if (hit.pos.x < -6 || hit.pos.x > 6) {
				return -1;
			}
			if (hit.pos.z < -14 || hit.pos.z > -2) {
				return -1;
			}
			hit.vertex = 5555;
			return t;
		}
	}
	return -1;
}


Plane::Plane(float amb[],
				float dif[], float dif2[], float spe[], float shine,
				float refl) : normal({0,1,0}) {
	normalize(&normal);
	mat_ambient[0] = amb[0];
	mat_ambient[1] = amb[1];
	mat_ambient[2] = amb[2];
	mat_diffuse[0] = dif[0];
	mat_diffuse[1] = dif[1];
	mat_diffuse[2] = dif[2];
	mat_diffuse2[0] = dif2[0];
	mat_diffuse2[1] = dif2[1];
	mat_diffuse2[2] = dif2[2];
	mat_specular[0] = spe[0];
	mat_specular[1] = spe[1];
	mat_specular[2] = spe[2];
	mat_shineness = shine;
	reflectance = refl;
}

/******************************************
 * computes a sphere normal - done for you
 ******************************************/
Vector Plane::getNormal(const IntersectionInfo &info) const {
	return normal;
}


float Plane::getDiffuse(const Point &p, int i) const {
	int x = ((p.x + 6) * 8) / 12;
	int z = ((p.z + 14) * 8) / 12;
	if (x % 2 == 0){
		if (z % 2 == 0) {
			return mat_diffuse2[i];
		} else {
			return mat_diffuse[i];
		}
	} else {
		if (z % 2 == 0) {
			return mat_diffuse[i];
		} else {
			return mat_diffuse2[i];
		}
	}

}
