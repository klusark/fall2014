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
float intersect_sphere(Point o, Vector u, Sphere *sph, Point *hit) {
	Vector oc = get_vec(o, sph->center);

	float oc2 = vec_dot(u,oc);
	float len = vec_len(oc);
	float tot = oc2*oc2 - len*len + (sph->radius * sph->radius);
	if (tot <= 0) {
		return -1;
	}
	float sq = sqrt(tot);
	float d1 = -oc2 + sq;
	float d2 = -oc2 - sq;
	float d = std::min(d1,d2);
	Vector sc = vec_scale(oc, d);
	Point p = get_point(o, sc);
	*hit = p;
	return d;
}

/*********************************************************************
 * This function returns a pointer to the sphere object that the
 * ray intersects first; NULL if no intersection. You should decide
 * which arguments to use for the function. For exmaple, note that you
 * should return the point of intersection to the calling function.
 **********************************************************************/
Sphere *intersect_scene() {
//
// do your thing here
//

	return nullptr;
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
Vector sphere_normal(Point q, Sphere *sph) {
	Vector rc;

	rc = get_vec(sph->center, q);
	normalize(&rc);
	return rc;
}
