#include "model.h"
#include <cstdio>
#include <cmath>

Model::Model(const std::string &filename) {
	FILE *f = fopen(filename.c_str(), "r");

	int verts, faces;
	fscanf(f, "# %d %d\n", &verts, &faces);

	for (int i = 0; i < verts; ++i) {
		float x,y,z;
		fscanf(f, "v %f %f %f\n", &x, &y, &z);
		_vertices.push_back({x,y,z-8});
	}

	for (int i = 0; i < faces; ++i) {
		int x,y,z;
		fscanf(f, "f %d %d %d\n", &x, &y, &z);
		x--;
		y--;
		z--;
		Vector norm;
		Vector v1 = _vertices[x];
		Vector v2 = _vertices[y];
		Vector v3 = _vertices[z];
		Vector u = v2 - v1;
		Vector v = v3 - v1;
		norm.x = u.y * v.z - u.z * v.y;
		norm.y = u.z * v.x - u.x * v.z;
		norm.z = u.x * v.y - u.y * v.x;

		_faces.push_back({x,y,z, norm});
	}
	fclose(f);

	mat_ambient[0] = 0.7;
	mat_ambient[1] = 0.7;
	mat_ambient[2] = 0.7;

	mat_diffuse[0] = 1;
	mat_diffuse[1] = 0;
	mat_diffuse[2] = 1;

	mat_specular[0] = 1;
	mat_specular[1] = 1;
	mat_specular[2] = 1;

	mat_shineness = 30;
	reflectance = 0.3;
}

// Moller-Trumbore intersection
int Model::intersect(const Vector &ray, const Vector &o, Point &out) {
	int size = _faces.size();
	for (int i = 0; i < size; ++i) {
		const Face &f = _faces[i];
		Vector v1 = _vertices[f.x];
		Vector v2 = _vertices[f.y];
		Vector v3 = _vertices[f.z];

		Vector e1 = v2-v1;
		Vector e2 = v3-v1;
		Vector p = vec_cross(ray, e2);
		float det = vec_dot(e1, p);

		if (fabs(det) < 0.000001f) {
			continue;
		}
		float inv_det = 1.f/det;

		Vector t = o - v1;

		float u = vec_dot(t, p) * inv_det;

		if (u < 0.f || u > 1.f) {
			continue;
		}

		Vector q = vec_cross(t, e1);
		float v = vec_dot(ray, q) * inv_det;

		if (v < 0.f || v + u > 1.f) {
			continue;
		}
		float t2 = vec_dot(e2, q) * inv_det;
		if (t2 > 0.0001f) {
			Vector sc = ray * t2;
			out.x = o.x + sc.x;
			out.y = o.y + sc.y;
			out.z = o.z + sc.z;
			return i;
		}
	}
	return -1;
}

Vector Model::getNormal(int i) {
	return _faces[i].norm;
}
