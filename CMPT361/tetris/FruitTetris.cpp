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
std::uniform_int_distribution<int> rot_distribution(0, 6);

// colors
vec4 white  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 black  = vec4(0.0, 0.0, 0.0, 1.0);

vec4 colours[] = {
	vec4(1.0, 0.5, 0.0, 1.0), // orange
	vec4(1.0, 0.0, 0.0, 1.0), // red
	vec4(0.0, 1.0, 0.0, 1.0), // green
	vec4(0.5, 0.0, 0.5, 1.0), // purple
	vec4(1.0, 1.0, 0.0, 1.0), // yellow
};
std::uniform_int_distribution<int> colour_distribution(0, 4);

//board[x][y] represents whether the cell (x,y) is occupied
bool board[10][20];

const int BOARD_SIZE = 10*20*2*3;

//An array containing the colour of each of the 10*20*2*3 vertices that make up the board
//Initially, all will be set to black. As tiles are placed, sets of 6 vertices (2 triangles; 1 square)
//will be set to the appropriate colour in this array before updating the corresponding VBO
vec4 boardcolours[BOARD_SIZE];

// location of vertex attributes in the shader program
GLuint vPosition;
GLuint vColor;

// locations of uniform variables in shader program
GLuint locxsize;
GLuint locysize;

// VAO and VBO
GLuint vaoIDs[3]; // One VAO for each object: the grid, the board, the current piece
GLuint vboIDs[6]; // Two Vertex Buffer Objects for each VAO (specifying vertex positions and colours, respectively)

const static int movetime = 200;

bool checkCollide();

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

// When the current tile is moved or rotated (or created), update the VBO
// containing its vertex position data
void updateTile() {
	// Bind the VBO containing current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);

	// For each of the 4 'cells' of the tile,
	for (int i = 0; i < 4; i++) {
		// Calculate the grid coordinates of the cell
		GLfloat x = tilepos.x + tile[i].x;
		GLfloat y = tilepos.y + tile[i].y;

		// Create the 4 corners of the square - these vertices are using location in pixels
		// These vertices are later converted by the vertex shader
		vec4 p1 = vec4(33.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1);
		vec4 p2 = vec4(33.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);
		vec4 p3 = vec4(66.0 + (x * 33.0), 33.0 + (y * 33.0), .4, 1);
		vec4 p4 = vec4(66.0 + (x * 33.0), 66.0 + (y * 33.0), .4, 1);

		// Two points are used by two triangles each
		vec4 newpoints[6] = {p1, p2, p3, p2, p3, p4};

		// Put new data in the VBO
		glBufferSubData(GL_ARRAY_BUFFER, i*6*sizeof(vec4), 6*sizeof(vec4), newpoints);
	}

	glBindVertexArray(0);
}

void updateBoard() {
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[3]);
	glBufferData(GL_ARRAY_BUFFER, BOARD_SIZE * sizeof(vec4), boardcolours, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
}

//------------------------------------------------------------------------------

// Called at the start of play and every time a tile is placed
void newtile() {
	rot = 0;
	colourRot = 0;
	tilepos = vec2(5, 20); // Put the tile at the top of the board
	tiletype = rot_distribution(generator);
	// Update the geometry VBO of current tile
	for (int i = 0; i < 4; i++) {
		// Get the 4 pieces of the new tile
		tile[i] = allRotationsLshape[tiletype][i];
	}
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
		if (board[(int)newpos.x][(int)newpos.y]) {
			gameDone = true;
		}
	}
	updateTile();

	// Update the color VBO of current tile
	vec4 newcolours[24];
	for (int i = 0; i < 4; ++i) {
		tilecolour[i] = colours[colour_distribution(generator)];
	}
	for (int i = 0; i < 24; i++) {
		// TODO: Randomize
		newcolours[i] = tilecolour[i/6];
	}
	// Bind the VBO containing current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	// Put the colour data in the VBO
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(newcolours), newcolours);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

//------------------------------------------------------------------------------

void initGrid()
{
	// ***Generate geometry data
	vec4 gridpoints[64]; // Array containing the 64 points of the 32 total lines to be later put in the VBO
	vec4 gridcolours[64]; // One colour per vertex
	// Vertical lines
	for (int i = 0; i < 11; i++){
		gridpoints[2*i] = vec4((33.0 + (33.0 * i)), 33.0, 0, 1);
		gridpoints[2*i + 1] = vec4((33.0 + (33.0 * i)), 693.0, 0, 1);

	}
	// Horizontal lines
	for (int i = 0; i < 21; i++){
		gridpoints[22 + 2*i] = vec4(33.0, (33.0 + (33.0 * i)), 0, 1);
		gridpoints[22 + 2*i + 1] = vec4(363.0, (33.0 + (33.0 * i)), 0, 1);
	}
	// Make all grid lines white
	for (int i = 0; i < 64; i++)
		gridcolours[i] = white;


	// *** set up buffer objects
	// Set up first VAO (representing grid lines)
	glBindVertexArray(vaoIDs[0]); // Bind the first VAO
	glGenBuffers(2, vboIDs); // Create two Vertex Buffer Objects for this VAO (positions, colours)

	// Grid vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]); // Bind the first grid VBO (vertex positions)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridpoints, GL_STATIC_DRAW); // Put the grid points in the VBO
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition); // Enable the attribute

	// Grid vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]); // Bind the second grid VBO (vertex colours)
	glBufferData(GL_ARRAY_BUFFER, 64*sizeof(vec4), gridcolours, GL_STATIC_DRAW); // Put the grid colours in the VBO
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor); // Enable the attribute
}


void initBoard()
{
	// *** Generate the geometric data
	vec4 boardpoints[BOARD_SIZE];
	for (int i = 0; i < BOARD_SIZE; i++) {
		// Let the empty cells on the board be black
		boardcolours[i] = black;
	}
	// Each cell is a square (2 triangles with 6 vertices)
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 10; j++) {
			vec4 p1 = vec4(33.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p2 = vec4(33.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);
			vec4 p3 = vec4(66.0 + (j * 33.0), 33.0 + (i * 33.0), .5, 1);
			vec4 p4 = vec4(66.0 + (j * 33.0), 66.0 + (i * 33.0), .5, 1);

			// Two points are reused
			boardpoints[6*(10*i + j)    ] = p1;
			boardpoints[6*(10*i + j) + 1] = p2;
			boardpoints[6*(10*i + j) + 2] = p3;
			boardpoints[6*(10*i + j) + 3] = p2;
			boardpoints[6*(10*i + j) + 4] = p3;
			boardpoints[6*(10*i + j) + 5] = p4;
		}
	}

	// Initially no cell is occupied
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 20; j++) {
			board[i][j] = false;
		}
	}


	// *** set up buffer objects
	glBindVertexArray(vaoIDs[1]);
	glGenBuffers(2, &vboIDs[2]);

	// Grid cell vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[2]);
	glBufferData(GL_ARRAY_BUFFER, BOARD_SIZE * sizeof(vec4), boardpoints, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Grid cell vertex colours
	updateBoard();
	glEnableVertexAttribArray(vColor);
}

// No geometry for current tile initially
void initCurrentTile() {
	glBindVertexArray(vaoIDs[2]);
	glGenBuffers(2, &vboIDs[4]);

	// Current tile vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[4]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);

	// Current tile vertex colours
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[5]);
	glBufferData(GL_ARRAY_BUFFER, 24*sizeof(vec4), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vColor);
}

void startGame() {
	gameDone = false;
	// Initialize the grid, the board, and the current tile
	initGrid();
	initBoard();
	initCurrentTile();

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
	glGenVertexArrays(3, &vaoIDs[0]);

	// The location of the uniform variables in the shader program
	locxsize = glGetUniformLocation(program, "xsize");
	locysize = glGetUniformLocation(program, "ysize");

	// set to default
	glBindVertexArray(0);
	glClearColor(0, 0, 0, 0);


	startGame();
}

void setSpace(int x, int y, bool occupied, const vec4 &colour) {
	board[x][y] = occupied;
	for (int j = 0; j < 6; ++j) {
		int pos = (y * 10 + x) * 6 + j;
		boardcolours[pos] = colour;
	}
}

bool getOccupied(int x, int y) {
	return board[x][y];
}

vec4 getColour(int x, int y) {
	return boardcolours[(y * 10 + x) * 6];
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

int timeElapsed = 0;
int timeSinceMove = 0;
void updateGame() {
	if (gameDone) {
		return;
	}
	int newtime = glutGet(GLUT_ELAPSED_TIME);
	int frametime = newtime - timeElapsed;
	timeSinceMove += frametime;
	if (timeSinceMove > movetime) {
		tilepos.y -= 1;
		updateTile();
		timeSinceMove %= movetime;
	}

	if (tileLanded()) {
		tilepos.y += 1;
		addTileToBoard();
		checkBoard();
		tilepos.y = 2;
		newtile();
	}

	timeElapsed = newtime;
}

// Draws the game
void display() {


	glClear(GL_COLOR_BUFFER_BIT);


	// x and y sizes are passed to the shader program to maintain shape of the
	// vertices on screen
	glUniform1i(locxsize, xsize);
	glUniform1i(locysize, ysize);

	// Bind the VAO representing the grid cells (to be drawn first)
	glBindVertexArray(vaoIDs[1]);
	// Draw the board
	glDrawArrays(GL_TRIANGLES, 0, BOARD_SIZE);

	glBindVertexArray(vaoIDs[2]); // Bind the VAO representing the current tile (to be drawn on top of the board)
	glDrawArrays(GL_TRIANGLES, 0, 24); // Draw the current tile (8 triangles)

	glBindVertexArray(vaoIDs[0]); // Bind the VAO representing the grid lines (to be drawn on top of everything else)
	glDrawArrays(GL_LINES, 0, 64); // Draw the grid lines (21+11 = 32 lines)


	glutSwapBuffers();
}

//------------------------------------------------------------------------------

// Reshape callback will simply change xsize and ysize variables, which are
// passed to the vertex shader to keep the game the same from stretching if the
// window is stretched
void reshape(GLsizei w, GLsizei h) {
	xsize = w;
	ysize = h;
	glViewport(0, 0, w, h);
}

//------------------------------------------------------------------------------

const static int LEFT = 100;
const static int UP = 101;
const static int RIGHT = 102;
const static int DOWN = 103;

// Handle arrow key keypresses
void special(int key, int x, int y) {
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
	} else if (key == DOWN) {
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
				colourRot = (colourRot + 1) % 4;
				updateRot();
				updateTile();
				break;
		}
		case 033: // Both escape key and 'q' cause the game to exit
		case 'q':
			exit (EXIT_SUCCESS);
			break;
		case 'r': // 'r' key restarts the game
			restart();
			break;
	}
	glutPostRedisplay();
}

//-------------------------------------------------------------------------------------------------------------------

void idle(void) {
	glutPostRedisplay();
	updateGame();
}

//-------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(xsize, ysize);
	glutInitWindowPosition(680, 178); // Center the game window (well, on a 1920x1080 display)
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
