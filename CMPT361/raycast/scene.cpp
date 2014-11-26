//
// this provide functions to set up the scene
//
#include <stdio.h>

#include "sphere.h"
#include "plane.h"
#include "raycast.h"
#include "model.h"

//////////////////////////////////////////////////////////////////////////

/*******************************************
 * set up the default scene - DO NOT CHANGE
 *******************************************/
void set_up_lights() {
	// set background color
	background_clr.r = 0.5;
	background_clr.g = 0.05;
	background_clr.b = 0.8;

	// setup global ambient term
	global_ambient[0] = global_ambient[1] = global_ambient[2] = 0.2;

	// setup light 1
	light1.x = -2.0;
	light1.y = 5.0;
	light1.z = 1.0;
	light1_ambient[0] = light1_ambient[1] = light1_ambient[2] = 0.1;
	light1_diffuse[0] = light1_diffuse[1] = light1_diffuse[2] = 1.0;
	light1_specular[0] = light1_specular[1] = light1_specular[2] = 1.0;

	// set up decay parameters
	decay_a = 0.5;
	decay_b = 0.3;
	decay_c = 0.0;
}

void setup_plane() {
	if (check_on) {
		float chess_ambient[] = {0.9, 0.0, 1};
		float chess_diffuse[] = {1, 1, 1};
		float chess_diffuse2[] = {0, 0, 0};
		float chess_specular[] = {0, 0, 0};
		float chess_shineness = 0;
		float chess_reflectance = .4;
		scene.push_back(new Plane(chess_ambient, chess_diffuse, chess_diffuse2,
						chess_specular, chess_shineness, chess_reflectance,
						{-3, 0, -6}, {3, 0, 0}, {0,1,0}));
	}
}

void set_up_default_scene() {
	set_up_lights();

	// sphere 1
	Point sphere1_ctr = {1.5, -0.2, -3.2};
	float sphere1_rad = 1.23;
	float sphere1_ambient[] = {0.7, 0.7, 0.7};
	float sphere1_diffuse[] = {0.1, 0.5, 0.8};
	float sphere1_specular[] = {1.0, 1.0, 1.0};
	float sphere1_shineness = 10;
	float sphere1_reflectance = 0.4;
	scene.push_back(new Sphere(sphere1_ctr, sphere1_rad, sphere1_ambient,
					sphere1_diffuse, sphere1_specular, sphere1_shineness,
					sphere1_reflectance, 1));

	// sphere 2
	Point sphere2_ctr = {-1.5, 0.0, -3.5};
	float sphere2_rad = 1.5;
	float sphere2_ambient[] = {0.6, 0.6, 0.6};
	float sphere2_diffuse[] = {1.0, 0.0, 0.25};
	float sphere2_specular[] = {1.0, 1.0, 1.0};
	float sphere2_shineness = 6;
	float sphere2_reflectance = 0.3;
	scene.push_back(new Sphere(sphere2_ctr, sphere2_rad, sphere2_ambient,
					sphere2_diffuse, sphere2_specular, sphere2_shineness,
					sphere2_reflectance, 2));

	// sphere 3
	Point sphere3_ctr = {-0.35, 1.75, -2.25};
	float sphere3_rad = 0.5;
	float sphere3_ambient[] = {0.2, 0.2, 0.2};
	float sphere3_diffuse[] = {0.0, 1.0, 0.25};
	float sphere3_specular[] = {0.0, 1.0, 0.0};
	float sphere3_shineness = 30;
	float sphere3_reflectance = 0.3;
	scene.push_back(new Sphere(sphere3_ctr, sphere3_rad, sphere3_ambient,
					sphere3_diffuse, sphere3_specular, sphere3_shineness,
					sphere3_reflectance, 3));
	setup_plane();
}

/***************************************
 * You can create your own scene here
 ***************************************/
void set_up_user_scene() {
	set_up_lights();

	// sphere 1
	Point sphere1_ctr = {-.6, -0.2, -2.2};
	float sphere1_rad = 1.23;
	float sphere1_ambient[] = {0.7, 0.7, 0.7};
	float sphere1_diffuse[] = {0.1, 0.5, 0.8};
	float sphere1_specular[] = {1.0, 1.0, 1.0};
	float sphere1_shineness = 10;
	float sphere1_reflectance = 0.4;
	scene.push_back(new Sphere(sphere1_ctr, sphere1_rad, sphere1_ambient,
					sphere1_diffuse, sphere1_specular, sphere1_shineness,
					sphere1_reflectance, 1));

	// sphere 2
	Point sphere2_ctr = {1.5, 0, -7.5};
	float sphere2_rad = 1.5;
	float sphere2_ambient[] = {0.6, 0.6, 0.6};
	float sphere2_diffuse[] = {1.0, 0.0, 0.25};
	float sphere2_specular[] = {1.0, 1.0, 1.0};
	float sphere2_shineness = 6;
	float sphere2_reflectance = 0.3;
	scene.push_back(new Sphere(sphere2_ctr, sphere2_rad, sphere2_ambient,
					sphere2_diffuse, sphere2_specular, sphere2_shineness,
					sphere2_reflectance, 2));

	// sphere 3
	Point sphere3_ctr = {-0.75, 2.15, -4.25};
	float sphere3_rad = 0.5;
	float sphere3_ambient[] = {0.2, 0.2, 0.2};
	float sphere3_diffuse[] = {0.0, 1.0, 0.25};
	float sphere3_specular[] = {0.0, 1.0, 0.0};
	float sphere3_shineness = 30;
	float sphere3_reflectance = 0.3;
	scene.push_back(new Sphere(sphere3_ctr, sphere3_rad, sphere3_ambient,
					sphere3_diffuse, sphere3_specular, sphere3_shineness,
					sphere3_reflectance, 3));

	scene[0]->transparency = 0.5;
	scene[1]->transparency = 0.6;
	scene[2]->transparency = 0.9;

	setup_plane();

}


void set_up_chess_scene() {
	set_up_lights();

	scene.push_back(new Model("chess_pieces/chess_piece.smf", {-1, -0.5, -2}));
	scene.push_back(new Model("chess_pieces/chess_piece.smf", {0, -0.5, -2}));

	float chess_ambient[] = {0.9, 0.0, 1};
	float chess_diffuse[] = {1, 1, 1};
	float chess_diffuse2[] = {0, 0, 0};
	float chess_specular[] = {0, 0, 0};
	float chess_shineness = 0;
	float chess_reflectance = .4;
	scene.push_back(new Plane(chess_ambient, chess_diffuse, chess_diffuse2,
					chess_specular, chess_shineness, chess_reflectance,
					{-3, 0, -6}, {3, 0, 0}, {0,1,0}));
}
