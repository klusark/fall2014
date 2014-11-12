#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include <cstdio>

#include "raycast.h"
#include "global.h"
#include "sphere.h"

//
// Global variables
//
extern int win_width;
extern int win_height;

extern GLfloat frame[WIN_HEIGHT][WIN_WIDTH][3];

extern float image_width;
extern float image_height;

extern Point eye_pos;
extern float image_plane;
extern RGB_float background_clr;
extern RGB_float null_clr;

// light 1 position and color
extern Point light1;
extern float light1_ambient[3];
extern float light1_diffuse[3];
extern float light1_specular[3];

// global ambient term
extern float global_ambient[3];

// light decay parameters
extern float decay_a;
extern float decay_b;
extern float decay_c;

extern int shadow_on;
extern int step_max;

/////////////////////////////////////////////////////////////////////

/*********************************************************************
 * Phong illumination - you need to implement this!
 *********************************************************************/
RGB_float phong(Point q, Vector v, Vector norm, Sphere *sph, float distance) {
	float ip[3] = {0,0,0};
	for (int i = 0; i < 3; ++i) {
		ip[i] += global_ambient[i];
		ip[i] += sph->mat_ambient[i] * light1_ambient[i];

		float decay = 1/(decay_a + decay_b * distance + decay_c * distance * distance);
		float ds = 0;
		Vector lm = get_vec(q, light1);
		normalize(&lm);
		normalize(&v);
		ds += light1_diffuse[i] * sph->mat_diffuse[i] * vec_dot(lm, norm);
		float nlen = vec_len(norm);
		Vector r = vec_minus(v, vec_scale(norm, vec_dot(vec_scale(v, 2), norm)/(nlen * nlen)));
		ds += light1_specular[i] * sph->mat_specular[i] * pow(vec_dot(r, v), sph->mat_shineness);

		ip[i] += ds * decay;
	}
	RGB_float color = {ip[0], ip[1], ip[2]};
	return color;
}

/************************************************************************
 * This is the recursive ray tracer - you need to implement this!
 * You should decide what arguments to use.
 ************************************************************************/
RGB_float recursive_ray_trace(Point &pos, Vector &ray, int) {
	RGB_float color = background_clr;
	bool hit = false;
	for (auto *s : scene) {
		Point end;
		float val = intersect_sphere(pos, ray, s, &end);

		if (val != -1) {
			Vector norm = sphere_normal(end, s);
			Vector V = get_vec(end, pos);
			color = phong(end, V, norm, s, val);
			break;
		}
	}
	if (hit) {
		color = {1,1,1};
	}
//
// do your thing here
//
	return color;
}

/*********************************************************************
 * This function traverses all the pixels and cast rays. It calls the
 * recursive ray tracer and assign return color to frame
 *
 * You should not need to change it except for the call to the recursive
 * ray tracer. Feel free to change other parts of the function however,
 * if you must.
 *********************************************************************/
void ray_trace() {
	int i, j;
	float x_grid_size = image_width / float(win_width);
	float y_grid_size = image_height / float(win_height);
	float x_start = -0.5 * image_width;
	float y_start = -0.5 * image_height;
	RGB_float ret_color;
	Point cur_pixel_pos;
	Vector ray;

	// ray is cast through center of pixel
	cur_pixel_pos.x = x_start + 0.5 * x_grid_size;
	cur_pixel_pos.y = y_start + 0.5 * y_grid_size;
	cur_pixel_pos.z = image_plane;

	for (i=0; i<win_height; i++) {
		for (j=0; j<win_width; j++) {
			ray = get_vec(eye_pos, cur_pixel_pos);

			//
			// You need to change this!!!
			//
			// ret_color = recursive_ray_trace();

			// Parallel rays can be cast instead using below
			//
			ray.x = ray.y = 0;
			ray.z = -1.0;
			ret_color = recursive_ray_trace(cur_pixel_pos, ray, 1);

			// Checkboard for testing
			//RGB_float clr = {float(i/32), 0, float(j/32)};

			frame[i][j][0] = GLfloat(ret_color.r);
			frame[i][j][1] = GLfloat(ret_color.g);
			frame[i][j][2] = GLfloat(ret_color.b);

			cur_pixel_pos.x += x_grid_size;
		}

		cur_pixel_pos.y += y_grid_size;
		cur_pixel_pos.x = x_start;
	}
}
