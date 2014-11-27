#include "model.h"
#include <cstdio>
#include <cmath>

Model::Model(const std::string &filename, const Vector &off) : bbtop({0,0,0}), bbbottom({0,0,0}) {
	FILE *f = fopen(filename.c_str(), "r");

	int verts, faces;
	fscanf(f, "# %d %d\n", &verts, &faces);

	for (int i = 0; i < verts; ++i) {
		float x,y,z;
		fscanf(f, "v %f %f %f\n", &x, &y, &z);
		x += off.x;
		y += off.y;
		z += off.z;
		if (i == 0 || x > bbtop.x) {
			bbtop.x = x;
		}
		if (i == 0 || y > bbtop.y) {
			bbtop.y = y;
		}
		if (i == 0 || z > bbtop.z) {
			bbtop.z = z;
		}
		if (i == 0 || x < bbbottom.x) {
			bbbottom.x = x;
		}
		if (i == 0 || y < bbbottom.y) {
			bbbottom.y = y;
		}
		if (i == 0 || z < bbbottom.z) {
			bbbottom.z = z;
		}

		_vertices.push_back({x,y,z});
	}

	for (int i = 0; i < faces; ++i) {
		int x,y,z;
		fscanf(f, "f %d %d %d\n", &x, &y, &z);
		x--;
		y--;
		z--;
		//Based on https://www.opengl.org/wiki/Calculating_a_Surface_Normal
		Vector norm;
		Vector v1 = _vertices[x];
		Vector v2 = _vertices[y];
		Vector v3 = _vertices[z];
		Vector u = v2 - v3;
		Vector v = v1 - v3;
		norm = normalize(cross(v,u));

		_faces.push_back({x,y,z, norm});
	}
	fclose(f);

	mat_ambient[0] = 0.7;
	mat_ambient[1] = 0.7;
	mat_ambient[2] = 0.7;

	mat_diffuse[0] = 0;
	mat_diffuse[1] = 0;
	mat_diffuse[2] = 1;

	mat_specular[0] = 1;
	mat_specular[1] = 1;
	mat_specular[2] = 1;

	mat_shineness = 30;
	reflectance = 0.5;
	transparency = 0.5;
}

// Moller-Trumbore intersection
float Model::intersect(const Point &r, const Vector &ray, IntersectionInfo &out) const {

	Vector o = {r.x, r.y, r.z};
	Vector lb = bbtop;
	Vector rt = bbbottom;

	// based on https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms/18459#18459
	// r.dir is unit direction vector of ray
	Vector dirfrac;
	dirfrac.x = 1.0f / ray.x;
	dirfrac.y = 1.0f / ray.y;
	dirfrac.z = 1.0f / ray.z;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	// r.org is origin of ray
	float t1 = (lb.x - o.x)*dirfrac.x;
	float t2 = (rt.x - o.x)*dirfrac.x;
	float t3 = (lb.y - o.y)*dirfrac.y;
	float t4 = (rt.y - o.y)*dirfrac.y;
	float t5 = (lb.z - o.z)*dirfrac.z;
	float t6 = (rt.z - o.z)*dirfrac.z;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
	if (tmax < 0) {
		return -1;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax) {
		return -1;
	}

	float closest = -1;
	bool notfound = true;
	int size = _faces.size();
	for (int i = 0; i < size; ++i) {
		const Face &f = _faces[i];
		Vector v3 = _vertices[f.x];
		Vector v2 = _vertices[f.y];
		Vector v1 = _vertices[f.z];

		Vector e1 = v2-v1;
		Vector e2 = v3-v1;
		Vector p = cross(ray, e2);
		float det = dot(e1, p);

		if (fabs(det) < 0.000001f) {
			continue;
		}
		float inv_det = 1.f/det;

		Vector t = o - v1;

		float u = dot(t, p) * inv_det;

		if (u < 0.f || u > 1.f) {
			continue;
		}

		Vector q = cross(t, e1);
		float v = dot(ray, q) * inv_det;

		if (v < 0.f || v + u > 1.f) {
			continue;
		}
		float t2 = dot(e2, q) * inv_det;
		if (t2 > 0.0001f) {
			if (notfound || t2 < closest) {
				notfound = false;
				Vector sc = ray * t2;
				out.pos.x = o.x + sc.x;
				out.pos.y = o.y + sc.y;
				out.pos.z = o.z + sc.z;
				out.vertex = i;
				closest = t2;
			}
		}
	}
	return closest;
}

Vector Model::getNormal(const IntersectionInfo &info) const {
	return _faces[info.vertex].norm;
}
