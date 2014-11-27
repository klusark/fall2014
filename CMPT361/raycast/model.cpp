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
		if (x > bbtop.x) {
			bbtop.x = x;
		}
		if (y > bbtop.y) {
			bbtop.y = y;
		}
		if (z > bbtop.z) {
			bbtop.z = z;
		}
		if (x < bbbottom.x) {
			bbbottom.x = x;
		}
		if (y < bbbottom.y) {
			bbbottom.y = y;
		}
		if (z < bbbottom.z) {
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
	reflectance = 0.3;
}

// Moller-Trumbore intersection
float Model::intersect(const Point &r, const Vector &ray, IntersectionInfo &out) const {

	Vector o = {r.x, r.y, r.z};
	Vector t1 = bbbottom - o;
	Vector t2 = bbtop - o;

	float tx1 = t1.x*ray.x;
	float tx2 = t2.x*ray.x;

	float tmin = std::min(tx1, tx2);
	float tmax = std::max(tx1, tx2);

	float ty1 = t1.y*ray.y;
	float ty2 = t2.y*ray.y;

	tmin = std::max(tmin, std::min(ty1, ty2));
	tmax = std::min(tmax, std::max(ty1, ty2));

	float tz1 = t1.z*ray.z;
	float tz2 = t2.z*ray.z;

	tmin = std::max(tmin, std::min(tz1, tz2));
	tmax = std::min(tmax, std::max(tz1, tz2));


	if (tmax <= tmin) {
		return -1;
	}

	float closest = -1;
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
			if (t2 < closest || closest == -1) {
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
