/*
CMPT 361 Assignment 1 - FruitTetris implementation Sample Skeleton Code

- This is ONLY a skeleton code showing:
How to use multiple buffers to store different objects
An efficient scheme to represent the grids and blocks

- Compile and Run:
Type make in terminal, then type ./FruitTetris

This code is extracted from Connor MacLeod's (crmacleo@sfu.ca) assignment submission
by Rui Ma (ruim@sfu.ca) on 2014-03-04.

Modified in Sep 2014 by Honghua Li (honghual@sfu.ca).
*/

#include "include/Angel.h"
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <cstring>

using namespace std;


// xsize and ysize represent the window size - updated if window is reshaped to
// prevent stretching of the game
int xsize = 400;
int ysize = 720;

// current tile
vec2 tile[4]; // An array of 4 2d vectors representing displacement from a 'center' piece of the tile, on the grid
vec2 tilepos = vec2(5, 19); // The position of the current tile using grid coordinates ((0,0) is the bottom left corner)
vec4 tilecolour[4];
int colourRot = 0;
int rot = 0;
int tiletype = 0;
int anglex = 0;
int angley = 0;
int a1 = -65, a2 = 10;
int armx = 0, army = 0;
int grey = 0;
int tildrop = 9;

bool gameDone = false;

// An array storing all possible orientations of all possible tiles
// The 'tile' array will always be some element [i][j] of this array (an array of vec2)
vec2 allRotationsLshape[][4] = {
	{vec2( 0,  0), vec2( 1,  0), vec2( 1,  1), vec2( 0,  1)}, // Square
	{vec2( 0,  1), vec2( 0,  0), vec2( 0, -1), vec2( 0, -2)}, // Line
	{vec2( 0, -1), vec2( 0,  0), vec2( 0,  1), vec2( 1,  0)}, // Pyramid
	{vec2( 1,  0), vec2( 0,  0), vec2(-1,  0), vec2(-1,  1)}, // L
	{vec2(-1,  1), vec2( 0,  1), vec2( 0,  0), vec2( 0, -1)}, // L
	{vec2(-1, -1), vec2(-1,  0), vec2( 0,  0), vec2( 0,  1)}, // Squiggle
	{vec2( 0, -1), vec2( 0,  0), vec2(-1,  0), vec2(-1,  1)}, // Squiggle
};

std::default_random_engine generator;
std::uniform_int_distribution<int> type_distribution(0, 6);

// colors
vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black  = vec4(0.0, 0.0, 0.0, 0.0);

vec4 colours[] = {
	vec4(1.0, 0.5, 0.0, 1.0), // orange
	vec4(1.0, 0.0, 0.0, 1.0), // red
	vec4(0.0, 1.0, 0.0, 1.0), // green
	vec4(0.5, 0.0, 0.5, 1.0), // purple
	vec4(1.0, 1.0, 0.0, 1.0), // yellow
};
std::uniform_int_distribution<int> colour_distribution(0, 4);
std::uniform_int_distribution<int> rot_distribution(0, 3);

//board[x][y] represents whether the cell (x,y) is occupied
bool board[10][20];

bool attached = true;

const int TILE_VERTS = 2 * 3 * 6;
const int BOARD_SIZE = 10*20*TILE_VERTS;
const int LINES_SIZE = (11 * 2 + 21 * 2) * 2 + 11*21*2;

//An array containing the colour of each of the 10*20*2*3 vertices that make up the board
//Initially, all will be set to black. As tiles are placed, sets of 6 vertices (2 triangles; 1 square)
//will be set to the appropriate colour in this array before updating the corresponding VBO
vec4 boardcolours[BOARD_SIZE];

// location of vertex attributes in the shader program
GLuint vPosition;
GLuint vColor;

// locations of uniform variables in shader program
GLuint ModelView, Projection, Grey;

// VAO and VBO
GLuint vaoIDs[4]; // One VAO for each object: the grid, the board, the current piece
GLuint gridVAO, boardVAO, tileVAO, armVAO;
GLuint gridVBOs[2];
GLuint boardVBOs[2];
GLuint tileVBOs[2];
GLuint armVBOs[2];

const static int movetime = 200;

bool checkCollide();

bool getOccupied(int x, int y) {
	return board[x][y];
}

vec4 getColour(int x, int y) {
	return boardcolours[(y * 10 + x) * TILE_VERTS];
}

//------------------------------------------------------------------------------
bool updateRot() {
	for (int i = 0; i < 4; i++) {
		tile[i] = allRotationsLshape[tiletype][(i + colourRot) % 4];
		if (rot == 1 || rot == 3) {
			std::swap(tile[i].x, tile[i].y);
		}
		if (rot == 2 || rot == 3) {
			tile[i].x = -tile[i].x;
			tile[i].y = -tile[i].y;
		}
	}
	return !checkCollide();
}

void makeTileVerts(vec4 *loc, int x, int y) {
	vec4 p0 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), -16, 1);
	vec4 p1 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), -16, 1);
	vec4 p2 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), -16, 1);
	vec4 p3 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), -16, 1);
	vec4 p4 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), 16, 1);
	vec4 p5 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), 16, 1);
	vec4 p6 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), 16, 1);
	vec4 p7 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), 16, 1);

	vec4 newpoints[] = {
		p0, p1, p2,
		p2, p3, p0,

		p3, p2, p6,
		p6, p7, p3,

		p7, p6, p5,
		p5, p4, p7,

		p4, p5, p1,
		p1, p0, p4,

		p4, p0, p3,
		p3, p7, p4,

		p1, p5, p6,
		p6, p2, p1,

	};
	memcpy(loc, newpoints, sizeof(newpoints));
}

void makeCube(vec4 *loc) {
	vec4 p0 = vec4(-0.5, -0.5, -0.5, 1);
	vec4 p1 = vec4( 0.5, -0.5, -0.5, 1);
	vec4 p2 = vec4( 0.5,  0.5, -0.5, 1);
	vec4 p3 = vec4(-0.5,  0.5, -0.5, 1);
	vec4 p4 = vec4(-0.5, -0.5,  0.5, 1);
	vec4 p5 = vec4( 0.5, -0.5,  0.5, 1);
	vec4 p6 = vec4( 0.5,  0.5,  0.5, 1);
	vec4 p7 = vec4(-0.5,  0.5,  0.5, 1);

	vec4 newpoints[] = {
		p0, p1, p2,
		p2, p3, p0,

		p3, p2, p6,
		p6, p7, p3,

		p7, p6, p5,
		p5, p4, p7,

		p4, p5, p1,
		p1, p0, p4,

		p4, p0, p3,
		p3, p7, p4,

		p1, p5, p6,
		p6, p2, p1,

	};
	memcpy(loc, newpoints, sizeof(newpoints));
}

// When the current tile is moved or rotated (or created), update the VBO
// containing its vertex position data
void updateTile() {
	// Bind the VBO containing current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, tileVBOs[0]);

	// For each of the 4 'cells' of the tile,
	for (int i = 0; i < 4; i++) {
		// Calculate the grid coordinates of the cell
		GLfloat x = tilepos.x + tile[i].x;
		GLfloat y = tilepos.y + tile[i].y;

		// Create the 4 corners of the square - these vertices are using location in pixels
		// These vertices are later converted by the vertex shader
		vec4 newpoints[TILE_VERTS];
		makeTileVerts(newpoints, x, y);

		// Put new data in the VBO
		glBufferSubData(GL_ARRAY_BUFFER, i*TILE_VERTS*sizeof(vec4), TILE_VERTS*sizeof(vec4),
						newpoints);
	}
}

void updateBoard() {
	glBindVertexArray(boardVAO);
	glBindBuffer(GL_ARRAY_BUFFER, boardVBOs[1]);
	glBufferData(GL_ARRAY_BUFFER, BOARD_SIZE * sizeof(vec4), boardcolours,
					GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
}

//------------------------------------------------------------------------------

// Called at the start of play and every time a tile is placed
void newtile() {
	tildrop = 9;
	attached = true;
	rot = rot_distribution(generator);
	colourRot = 0;
	tilepos = vec2(armx, army); // Put the tile at the top of the board
	tiletype = type_distribution(generator);
	// Update the geometry VBO of current tile
	for (int i = 0; i < 4; i++) {
		// Get the 4 pieces of the new tile
		tile[i] = allRotationsLshape[tiletype][i];
	}
	updateRot();
	for (int i = 0; i < 4; ++i) {
		vec2 newpos = tilepos;
		newpos += tile[i];
		while (newpos.y >= 20) {
			tilepos.y -= 1;
			newpos.y -= 1;
		}
	}
	for (int i = 0; i < 4; ++i) {
		vec2 newpos = tilepos;
		newpos += tile[i];
		if (getOccupied(newpos.x, newpos.y)) {
			gameDone = true;
		}
	}
	updateTile();

	std::set<int> selected_colours;
	// Update the color VBO of current tile
	vec4 newcolours[TILE_VERTS * 4];
	for (int i = 0; i < 4; ++i) {
		int selection = colour_distribution(generator);
		while (selected_colours.find(selection) != selected_colours.end()) {
			selection = colour_distribution(generator);
		}
		selected_colours.insert(selection);
		tilecolour[i] = colours[selection];
	}
	for (int i = 0; i < TILE_VERTS * 4; i++) {
		newcolours[i] = tilecolour[i/TILE_VERTS];
	}
	// Bind the VBO containing current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, tileVBOs[1]);
	// Put the colour data in the VBO
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//------------------------------------------------------------------------------

void initGrid()
{
	// ***Generate geometry data
	// Array containing the 64 points of the 32 total lines to be later put in the VBO
	vec4 gridpoints[LINES_SIZE];
	vec4 gridcolours[LINES_SIZE]; // One colour per vertex
	// Vertical lines
	for (int i = 0; i < 11; i++){
		gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, -17, 1);
		gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, -17, 1);

	}
	// Horizontal lines
	for (int i = 0; i < 21; i++){
		gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), -17, 1);
		gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), -17, 1);
	}
	for (int i = 0; i < 11; i++){
		gridpoints[64 + 2*i] = vec4((33.0 + (33.0 * i)), 33.0, 17, 1);
		gridpoints[64 + 2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 17, 1);

	}
	// Horizontal lines
	for (int i = 0; i < 21; i++){
		gridpoints[86 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 17, 1);
		gridpoints[86 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 17, 1);
	}
	for (int x = 0; x < 11; ++x) {
		for (int y = 0; y < 21; ++y) {
			gridpoints[128 + ((x * 21) + y)*2] = vec4(33.0 + (33.0 * x), (33.0 + (33.0 * y)), 17, 1);
			gridpoints[128 + ((x * 21) + y)*2 + 1] = vec4(33.0 + (33.0 * x), (33.0 + (33.0 * y)), -17, 1);
		}
	}
	// Make all grid lines white
	for (int i = 0; i < LINES_SIZE; i++)
		gridcolours[i] = white;


	// *** set up buffer objects
	// Set up first VAO (representing grid lines)
	// Bind the first VAO
	glBindVertexArray(gridVAO);
	// Create two Vertex Buffer Objects for this VAO (positions, colours)

	// Grid vertex positions
	// Bind the first grid VBO (vertex positions)
	glBindBuffer(GL_ARRAY_BUFFER, gridVBOs[0]);
	// Put the grid points in the VBO
	glBufferData(GL_ARRAY_BUFFER, LINES_SIZE*sizeof(vec4), gridpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition); // Enable the attribute

	// Grid vertex colours
	// Bind the second grid VBO (vertex colours)
	glBindBuffer(GL_ARRAY_BUFFER, gridVBOs[1]);
	// Put the grid colours in the VBO
	glBufferData(GL_ARRAY_BUFFER, LINES_SIZE*sizeof(vec4), gridcolours, GL_STATIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor); // Enable the attribute
}

void initArm()
{
	vec4 gridpoints[TILE_VERTS];
	vec4 gridcolours[TILE_VERTS];
	makeCube(gridpoints);

	for (int i = 0; i < TILE_VERTS; i++)
		gridcolours[i] = vec4(0,0,0.5,1);


	// *** set up buffer objects
	// Set up first VAO (representing grid lines)
	// Bind the first VAO
	glBindVertexArray(armVAO);
	// Create two Vertex Buffer Objects for this VAO (positions, colours)

	// Grid vertex positions
	// Bind the first grid VBO (vertex positions)
	glBindBuffer(GL_ARRAY_BUFFER, armVBOs[0]);
	// Put the grid points in the VBO
	glBufferData(GL_ARRAY_BUFFER, TILE_VERTS*sizeof(vec4), gridpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition); // Enable the attribute

	// Grid vertex colours
	// Bind the second grid VBO (vertex colours)
	glBindBuffer(GL_ARRAY_BUFFER, armVBOs[1]);
	// Put the grid colours in the VBO
	glBufferData(GL_ARRAY_BUFFER, TILE_VERTS*sizeof(vec4), gridcolours, GL_STATIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor); // Enable the attribute
}

void clearBoard() {
	for (int i = 0; i < BOARD_SIZE; i++) {
		// Let the empty cells on the board be black
		boardcolours[i] = black;
	}

	// Initially no cell is occupied
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 20; j++) {
			board[i][j] = false;
		}
	}
}

void initBoard()
{
	// *** Generate the geometric data
	vec4 boardpoints[BOARD_SIZE];

	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 10; j++) {
			makeTileVerts(&boardpoints[TILE_VERTS*(10*i + j)], j, i);
		}
	}


	clearBoard();

	// *** set up buffer objects
	glBindVertexArray(boardVAO);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, boardVBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, BOARD_SIZE * sizeof(vec4), boardpoints,
					GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	updateBoard();
	glEnableVertexAttribArray(vColor);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

// No geometry for current tile initially
void initCurrentTile() {
	glBindVertexArray(tileVAO);

	// Current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, tileVBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, TILE_VERTS*4*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, tileVBOs[1]);
	glBufferData(GL_ARRAY_BUFFER, TILE_VERTS*4*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

void startGame() {
	gameDone = false;

	clearBoard();
	updateBoard();

	// Game initialization
	newtile(); // create new next tile
}

void init() {
	// Load shaders and use the shader program
	GLuint program = InitShader("vshader.glsl", "fshader.glsl");
	glUseProgram(program);

	// Get the location of the attributes (for glVertexAttribPointer() calls)
	vPosition = glGetAttribLocation(program, "vPosition");
	vColor = glGetAttribLocation(program, "vColor");

	// Create 3 Vertex Array Objects, each representing one 'object'. Store the
	// names in array vaoIDs
	glGenVertexArrays(4, vaoIDs);
	tileVAO = vaoIDs[0];
	gridVAO = vaoIDs[1];
	boardVAO = vaoIDs[2];
	armVAO = vaoIDs[3];

	glGenBuffers(2, tileVBOs);
	glGenBuffers(2, gridVBOs);
	glGenBuffers(2, boardVBOs);
	glGenBuffers(2, armVBOs);


	// The location of the uniform variables in the shader program
	ModelView = glGetUniformLocation(program, "ModelView");
	Projection = glGetUniformLocation(program, "Projection");
	Grey = glGetUniformLocation(program, "grey");

	// set to default
	glClearColor(0, 0, 0, 0);

	// Initialize the grid, the board, and the current tile
	initGrid();
	initBoard();
	initCurrentTile();
	initArm();

	startGame();
}

void setSpace(int x, int y, bool occupied, const vec4 &colour) {
	board[x][y] = occupied;
	for (int j = 0; j < TILE_VERTS; ++j) {
		int pos = (y * 10 + x) * TILE_VERTS + j;
		boardcolours[pos] = colour;
	}
}

bool operator == (const vec4 &a, const vec4 &b) {
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

void checkBoard() {
	bool needUpdate = false;
	// check the rows
	for (int y = 0; y < 20; ++y) {
		bool allOccupied = true;
		for (int x = 0; x < 10; ++x) {
			if (!board[x][y]) {
				allOccupied = false;
				break;
			}
		}
		if (allOccupied) {
			needUpdate = true;
			for (int x = 0; x < 10; ++x) {
				setSpace(x, y, false, black);
			}
			for (int y2 = y; y2 < 19; ++y2) {
				for (int x = 0; x < 10; ++x) {
					setSpace(x, y2, getOccupied(x, y2+1), getColour(x, y2+1));
				}
			}
			//We need to check the current line again to make sure that it
			//wasn't filled twice
			--y;
		}
	}
	for (int y = 0; y < 20; ++y) {
		for (int x = 1; x < 9; ++x) {
			vec4 c = getColour(x, y);
			if (!(c == black) && c == getColour(x - 1, y) && c == getColour(x + 1, y)) {
				needUpdate = true;
				for (int z = x-1; z <= x+1; ++z) {
					setSpace(z, y, false, black);
				}
				for (int y2 = y; y2 < 19; ++y2) {
					for (int x2 = x-1; x2 <= x+1; ++x2) {
						setSpace(x2, y2, getOccupied(x2, y2+1), getColour(x2, y2+1));
					}
				}
			}
		}
	}
	for (int x = 0; x < 10; ++x) {
		for (int y = 1; y < 19; ++y) {
			vec4 c = getColour(x, y);
			if (!(c == black) && c == getColour(x, y-1) && c == getColour(x, y+1)) {
				needUpdate = true;
				for (int z = y-1; z <= y+1; ++z) {
					setSpace(x, z, false, black);
				}
				for (int y2 = y; y2 < 17; ++y2) {
					setSpace(x, y2-1, getOccupied(x, y2+2), getColour(x, y2+2));
				}
			}
		}
	}
	if (needUpdate) {
		updateBoard();
	}
}



// Starts the game over - empties the board, creates new tiles, resets line
// counters
void restart() {
	startGame();
}
//------------------------------------------------------------------------------

bool tileLanded() {
	for (int i = 0; i < 4; ++i) {
		vec2 newpos = tilepos;
		newpos += tile[i];
		if (newpos.y < 0) {
			return true;
		}
		if (board[(int)newpos.x][(int)newpos.y]) {
			return true;
		}

	}
	return false;
}

bool checkCollide() {
	for (int i = 0; i < 4; ++i) {
		vec2 newpos = tilepos;
		newpos += tile[i];
		if (newpos.x < 0 || newpos.x > 9) {
			return true;
		}
		if (newpos.y > 20) {
			return true;
		}
	}
	return tileLanded();
}

void addTileToBoard() {
	for (int i = 0; i < 4; ++i) {
		vec2 newpos = tilepos;
		newpos += tile[i];
		setSpace(newpos.x, newpos.y, true, tilecolour[i]);
	}
	updateBoard();
}

const GLfloat BASE_HEIGHT  = 32;
const GLfloat BASE_WIDTH   = 40;
const GLfloat ARM_HEIGHT = (22*33)/2;
const GLfloat ARM_WIDTH  = 16;

int timeElapsed = 0;
int timeSinceMove = 0;
void updateGame() {
	if (gameDone) {
		return;
	}
	int newtime = glutGet(GLUT_ELAPSED_TIME);
	int frametime = newtime - timeElapsed;
	timeSinceMove += frametime;
	if (!attached && timeSinceMove > movetime) {
		tilepos.y -= 1;
		updateTile();
		timeSinceMove %= movetime;
	}

	if (attached) {
		if (timeSinceMove > 1000) {
			tildrop -= 1;
			timeSinceMove %= 1000;
		}
		if (tildrop == 0) {
			attached = false;
		}
		// calculate the end of the arm
		mat4 v;
		v *= ( Translate(0.0, BASE_HEIGHT, 0.0) * RotateZ(a2) );
		v *= ( Translate(0.0, ARM_HEIGHT, 0.0) * RotateZ(a1) );
		v *= Translate(0, ARM_HEIGHT, 0);

		vec4 point(1,1,0,1);
		point = v * point;
		armx = (point.x - 40)/33;
		army = (point.y - 40)/33;
		tilepos.x = armx;
		tilepos.y = army;
		grey = (int)checkCollide();
		updateTile();
	} else {

		if (tileLanded()) {
			tilepos.y += 1;
			addTileToBoard();
			checkBoard();
			tilepos.y = 2;
			newtile();
		}
	}

	timeElapsed = newtime;
}

// Draws the game
void display() {


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// x and y sizes are passed to the shader program to maintain shape of the
	// vertices on screen
	mat4 model_view = Translate(233, 0, 0) * RotateY(anglex) * RotateX(angley) * Translate(-200, 0, 0);
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

	// Bind the VAO representing the grid cells (to be drawn first)
	glBindVertexArray(boardVAO);
	// Draw the board
	glDrawArrays(GL_TRIANGLES, 0, BOARD_SIZE);

	glUniform1i(Grey, grey);
	// Bind the VAO representing the current tile (to be drawn on top of the board)
	glBindVertexArray(tileVAO);
	// Draw the current tile (8 triangles)
	glDrawArrays(GL_TRIANGLES, 0, TILE_VERTS*4);
	glUniform1i(Grey, 0);

	// Bind the VAO representing the grid lines (to be drawn on top of everything else)
	glBindVertexArray(gridVAO);
	glDrawArrays(GL_LINES, 0, LINES_SIZE);

	glRasterPos3f(-0.9, 0.9, 0);
	glutBitmapCharacter(GLUT_BITMAP_8_BY_13, '0' + tildrop);

	glBindVertexArray(armVAO);

	model_view *= Translate(0, 33, 0);
	mat4 v = model_view * Translate(0, 0.5 * BASE_HEIGHT, 0) * Scale(BASE_WIDTH, BASE_HEIGHT, BASE_WIDTH);
	glUniformMatrix4fv( ModelView, 1, GL_TRUE, v );
	glDrawArrays(GL_TRIANGLES, 0, TILE_VERTS);

	model_view *= ( Translate(0.0, BASE_HEIGHT, 0.0) * RotateZ(a2) );
	v = model_view * Translate(0, 0.5 * ARM_HEIGHT, 0) * Scale(ARM_WIDTH, ARM_HEIGHT, ARM_WIDTH);
	glUniformMatrix4fv( ModelView, 1, GL_TRUE, v );
	glDrawArrays(GL_TRIANGLES, 0, TILE_VERTS);

	model_view *= ( Translate(0.0, ARM_HEIGHT, 0.0) * RotateZ(a1) );
	v = model_view * Translate(0, 0.5 * ARM_HEIGHT, 0) * Scale(ARM_WIDTH, ARM_HEIGHT, ARM_WIDTH);
	glUniformMatrix4fv( ModelView, 1, GL_TRUE, v );
	glDrawArrays(GL_TRIANGLES, 0, TILE_VERTS);



	glutSwapBuffers();
}

//------------------------------------------------------------------------------

// Reshape callback will simply change xsize and ysize variables, which are
// passed to the vertex shader to keep the game the same from stretching if the
// window is stretched
void reshape(GLsizei width, GLsizei height) {
	glViewport( 0, 0, width, height );

	GLfloat  left = 0, right = 500;
	GLfloat  bottom = 0, top = 500;
	GLfloat  zNear = -500.0, zFar = 500.0;

	GLfloat aspect = GLfloat(width)/height;

	if ( aspect > 1.0 ) {
		left *= aspect;
		right *= aspect;
	} else {
		bottom /= aspect;
		top /= aspect;
	}

	mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );
	glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );

	//model_view = mat4( 1.0 );  // An Identity matrix
}

//------------------------------------------------------------------------------

const static int LEFT = 100;
const static int UP = 101;
const static int RIGHT = 102;
const static int DOWN = 103;

// Handle arrow key keypresses
void special(int key, int x, int y) {
	if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
		if (key == LEFT) {
			anglex -= 5;
		} else if (key == RIGHT) {
			anglex += 5;
		}
		return;
	}

	if (gameDone) {
		return;
	}
	vec2 startpos = tilepos;
	if (key == LEFT) {
		tilepos.x -= 1;
	} else if (key == RIGHT) {
		tilepos.x += 1;
	} else if (key == UP) {
		int lastrot = rot;
		rot = (rot + 1) % 4;
		if (!updateRot()) {
			rot = lastrot;
			updateRot();
		}
	} else if (!attached && key == DOWN) {
		while (!tileLanded()) {
			tilepos.y -= 1;
		}
		tilepos.y += 1;
	}


	if (checkCollide()) {
		tilepos = startpos;
	} else {
		updateTile();
	}
}

//------------------------------------------------------------------------------

// Handles standard keypresses
void keyboard(unsigned char key, int x, int y) {
	switch(key) {
		if (!gameDone) {
			case ' ':
				if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
					colourRot = (colourRot + 1) % 4;
					updateRot();
					updateTile();
				} else {
					attached = false;
				}
				break;
		}
		case 033: // Both escape key and 'q' cause the game to exit
		case 'q':
			exit (EXIT_SUCCESS);
			break;
		case 'd':
			a1 -= 5;
			break;
		case 'a':
			a1 += 5;
			break;
		case 'w':
			a2 += 5;
			break;
		case 's':
			a2 -= 5;
			break;
		case 'r': // 'r' key restarts the game
			restart();
			break;
	}
	glutPostRedisplay();
}

//------------------------------------------------------------------------------

void idle(void) {
	glutPostRedisplay();
	updateGame();
}

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(xsize, ysize);

	// Center the game window (well, on a 1920x1080 display)
	glutInitWindowPosition(680, 178);
	glutCreateWindow("Fruit Tetris");
	glewInit();
	init();

	// Callback functions
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	glutMainLoop(); // Start main loop
	return 0;
}
