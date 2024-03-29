#include "sphere.h"
#include <stdlib.h>
#include <math.h>
#include <cstdio>
#include <algorithm>

/**********************************************************************
 * This function intersects a ray with a given sphere 'sph'. You should
 * use the parametric representation of a line and do the intersection.
 * The function should return the parameter value for the intersection,
 * which will be compared with others to determine which intersection
 * is closest. The value -1.0 is returned if there is no intersection
 *
 * If there is an intersection, the point of intersection should be
 * stored in the "hit" variable
 **********************************************************************/
float Sphere::intersect(const Point &o, const Vector &u, IntersectionInfo &hit) const {
	Vector oc = get_vec(center, o);

	float loc = dot(u, oc);
	float len = length(oc);
	float tot = loc * loc - len * len + (radius * radius);
	if (tot <= 0) {
		return -1;
	}
	float sq = sqrt(tot);
	float d1 = (-loc) + sq;
	float d2 = (-loc) - sq;
	float d = -1;
	if (d1 > 0.001 && d2 > 0.001) {
		d = std::min(d1,d2);
	} else if (d1 > 0.001) {
		d = d1;
	} else if (d2 > 0.001) {
		d = d2;
	}
	Vector sc = u * d;
	hit.pos = get_point(o, sc);
	return d;
}

Object::Object() {
	mat_ambient[0] = 0;
	mat_ambient[1] = 0;
	mat_ambient[2] = 0;
	mat_diffuse[0] = 0;
	mat_diffuse[1] = 0;
	mat_diffuse[2] = 0;
	mat_specular[0] = 0;
	mat_specular[1] = 0;
	mat_specular[2] = 0;
	mat_shineness = 0;
	reflectance = 0;
	transparency = 0;
}


/*****************************************************
 * This function adds a sphere into the sphere list
 *
 * You need not change this.
 *****************************************************/
Sphere::Sphere(Point ctr, float rad, float amb[],
				float dif[], float spe[], float shine,
				float refl, int sindex) {
	index = sindex;
	center = ctr;
	radius = rad;
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
Vector Sphere::getNormal(const IntersectionInfo &info) const {
	Vector rc;

	rc = get_vec(center, info.pos);
	return normalize(rc);
}
