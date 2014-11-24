#pragma once

#include <vector>
#include "sphere.h"
#include "global.h"

extern std::vector<Object *> scene;

extern int shadow_on;
extern int antialias_on;
extern int refract_on;
extern int reflect_on;
extern int check_on;
extern int step_max;


extern int win_width;
extern int win_height;

extern float frame[WIN_HEIGHT][WIN_WIDTH][3];

extern float image_width;
extern float image_height;

extern RGB_float background_clr;
extern RGB_float null_clr;

extern Point eye_pos;
extern float image_plane;

extern Point light1;
extern float light1_ambient[3];
extern float light1_diffuse[3];
extern float light1_specular[3];

extern float global_ambient[3];

// light decay parameters
extern float decay_a;
extern float decay_b;
extern float decay_c;

