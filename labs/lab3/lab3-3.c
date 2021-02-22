// Lab 3-3.

// Should work as is on Linux and Mac. MS Windows needs GLEW or glee.
// See separate Visual Studio version of my demos.
#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	// Linking hint for Lightweight IDE
	// uses framework Cocoa
#endif
#include <math.h>
#include <stdio.h>

#include "MicroGlut.h"
#include "GL_utilities.h"
#include "loadobj.h"
#include "LoadTGA.h"
#include "VectorUtils3.h"

// Globals

#define WIN_WIDTH 1000
#define WIN_HEIGHT 1000

// projection matrix

#define near 1.0
#define far 1000.0
#define right 0.5
#define left -0.5
#define top 0.5
#define bottom -0.5

const GLfloat projectionMatrix[] = {
	2.0f * near / (right - left), 0.0f, (right + left) / (right - left), 0.0f,
	0.0f, 2.0f * near / (top - bottom), (top + bottom) / (top - bottom), 0.0f,
	0.0f, 0.0f, -(far + near) / (far - near), -2 * far * near / (far - near),
	0.0f, 0.0f, -1.0f, 0.0f
};

// ground object

#define GROUND_SIZE 1000.0f
const GLfloat groundVertices[] = {
	-GROUND_SIZE, 0.0f, -GROUND_SIZE,
	-GROUND_SIZE, 0.0f, GROUND_SIZE,
	GROUND_SIZE, 0.0f, -GROUND_SIZE,

	GROUND_SIZE, 0.0f, -GROUND_SIZE,
	-GROUND_SIZE, 0.0f, GROUND_SIZE,
	GROUND_SIZE, 0.0f, GROUND_SIZE,
};
#define GROUND_TEX_SIZE 100.0f
const GLfloat groundTexCoords[] = {
	0.0f, 0.0f,
	0.0f, GROUND_TEX_SIZE,
	GROUND_TEX_SIZE, 0.0f,

	GROUND_TEX_SIZE, 0.0f,
	0.0f, GROUND_TEX_SIZE,
	GROUND_TEX_SIZE, GROUND_TEX_SIZE,
};
const int groundNumVertices = 6;

// Ground conisting of large square
GLuint groundVAO;

// Loaded models
typedef struct {
	Model *model;
	GLuint vao;
} ModelAndVAO;

ModelAndVAO windmillBlade;
ModelAndVAO windmillRoof;
ModelAndVAO windmillBalcony;
ModelAndVAO windmillWalls;
ModelAndVAO skybox;
ModelAndVAO teapot;
ModelAndVAO bunny;

// Textures
GLuint skyboxTex;
GLuint dirtTex;
GLuint grassTex;
GLuint concTex;

void loadModelAndVAO(GLuint program, const char *filename, ModelAndVAO *m) {
	m->model = LoadModel(filename);
	if (!m) {
		fprintf(stderr, "Model is NULL!\n");
		exit(1);
	}

	// Vertex array object
	glGenVertexArrays(1, &m->vao);

	// vertex buffer objects
	GLuint vertexBufferObjID;
	GLuint indexBufferObjID;
	GLuint normalBufferObjID;
	GLuint texCoordBufferObjID;

	glGenBuffers(1, &vertexBufferObjID);
	glGenBuffers(1, &indexBufferObjID);
	glGenBuffers(1, &normalBufferObjID);
	glGenBuffers(1, &texCoordBufferObjID);
	glBindVertexArray(m->vao);

	// VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, m->model->numVertices * 3 * sizeof(GLfloat), m->model->vertexArray, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "inPosition"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "inPosition"));
	printError("init positions");

	// VBO for normal data
	if (m->model->normalArray != NULL) {
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferObjID);
		glBufferData(GL_ARRAY_BUFFER, m->model->numVertices * 3 * sizeof(GLfloat), m->model->normalArray, GL_STATIC_DRAW);
		glVertexAttribPointer(glGetAttribLocation(program, "inNormal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(glGetAttribLocation(program, "inNormal"));
		printError("init normals");
	} else {
		fprintf(stderr, "Warning: model %s has no normals.\n", filename);
	}

	// VBO for texture coordinates
	if (m->model->texCoordArray != NULL) {
		glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferObjID);
		glBufferData(GL_ARRAY_BUFFER, m->model->numVertices * 2 * sizeof(GLfloat), m->model->texCoordArray, GL_STATIC_DRAW);
		glVertexAttribPointer(glGetAttribLocation(program, "inTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(glGetAttribLocation(program, "inTexCoord"));
		printError("init texture coords");
	} else {
		fprintf(stderr, "Warning: model %s has no texture coordinates.\n", filename);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObjID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->model->numIndices * sizeof(GLuint), m->model->indexArray, GL_STATIC_DRAW);
	printError("init indices");
}

void drawModelWithTex(const ModelAndVAO *m, GLuint texture) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(m->vao); // Select VAO
	glDrawElements(GL_TRIANGLES, m->model->numIndices, GL_UNSIGNED_INT, 0L);
}

void setModelMatrix(GLuint modelMatAttr, mat4 matrix) {
	glUniformMatrix4fv(modelMatAttr, 1, GL_TRUE, matrix.m);
}

void init(void)
{
	// Reference to shader program
	GLuint program;

	dumpInfo();

	// GL inits
	glClearColor(0.1, 0.1, 0.1, 0);
	glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
	printError("GL inits");

	// Load and compile shader
	program = loadShaders("lab3-3.vert", "lab3-3.frag");
	printError("init shader");

	// Load models and upload geometry to the GPU:
	loadModelAndVAO(program, "windmill/blade.obj", &windmillBlade);
	loadModelAndVAO(program, "windmill/windmill-balcony.obj", &windmillBalcony);
	loadModelAndVAO(program, "windmill/windmill-roof.obj", &windmillRoof);
	loadModelAndVAO(program, "windmill/windmill-walls.obj", &windmillWalls);
	loadModelAndVAO(program, "skybox.obj", &skybox);
	loadModelAndVAO(program, "bunnyplus.obj", &bunny);
	loadModelAndVAO(program, "teapot.obj", &teapot);

	// Vertex array object for ground
	glGenVertexArrays(1, &groundVAO);
	GLuint groundVBO, groundTexCoordBO;
	glGenBuffers(1, &groundVBO);
	glGenBuffers(1, &groundTexCoordBO);
	glBindVertexArray(groundVAO);

	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	glBufferData(GL_ARRAY_BUFFER, groundNumVertices * 3 * sizeof(GLfloat), groundVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "inPosition"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "inPosition"));
	printError("init positions for ground");

	glBindBuffer(GL_ARRAY_BUFFER, groundTexCoordBO);
	glBufferData(GL_ARRAY_BUFFER, groundNumVertices * 2 * sizeof(GLfloat), groundTexCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "inTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "inTexCoord"));
	printError("init texture coords for ground");

	// End of upload of geometry

	// Upload textures to GPU

	// Load textures

	LoadTGATextureSimple("dirt.tga", &dirtTex);
	LoadTGATextureSimple("grass.tga", &grassTex);
	LoadTGATextureSimple("conc.tga", &concTex);
	LoadTGATextureSimple("SkyBox512.tga", &skyboxTex);

	// Select texture 0
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "texUnit"), 0);

	// End of texture upload

	// Set projection matrix uniform
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, projectionMatrix);
	printError("init matrices");
}

// Variables related to viewer position
vec3 viewPos = {10.0f, 0.0f, 10.0f};
vec3 viewTarget = {0.0f, 0.0f, 0.0f};
float previousT = 0.0f;

void MouseFunc(int x, int y) {

}

void handleFlyingControls(vec3 up_vector, float delta) {
	delta *= 50.0f; // adjust speed
	// Fly around in the world, update view matrix for next frame
	const vec3 forward_vector = Normalize(VectorSub(viewTarget, viewPos));
	// cross product of two unit vectors is also unit
	const vec3 right_vector = Normalize(CrossProduct(forward_vector, up_vector));
	// Forward and backward
	if (glutKeyIsDown('w')) {
		viewPos = VectorAdd(viewPos, ScalarMult(forward_vector, delta));
	}
	if (glutKeyIsDown('s')) {
		viewPos = VectorSub(viewPos, ScalarMult(forward_vector, delta));
	}
	// Strafing left and right
	if (glutKeyIsDown('d')) {
		viewPos = VectorAdd(viewPos, ScalarMult(right_vector, delta));
	}
	if (glutKeyIsDown('a')) {
		viewPos = VectorSub(viewPos, ScalarMult(right_vector, delta));
	}
	// Up and down
	if (glutKeyIsDown(' ')) {
		viewPos = VectorAdd(viewPos, ScalarMult(up_vector, delta));
	}
	if (glutKeyIsDown('c')) {
		viewPos = VectorSub(viewPos, ScalarMult(up_vector, delta));
	}
	// Put view target 100 units in front of view pos
	viewTarget = VectorAdd(viewPos, ScalarMult(forward_vector, 100.0f));
	// Turning, should be done with mouse also
	if (glutKeyIsDown(GLUT_KEY_RIGHT)) {
		viewTarget = VectorAdd(viewTarget, ScalarMult(right_vector, delta));
	}
	if (glutKeyIsDown(GLUT_KEY_LEFT)) {
		viewTarget = VectorSub(viewTarget, ScalarMult(right_vector, delta));
	}
	if (glutKeyIsDown(GLUT_KEY_UP)) {
		viewTarget = VectorAdd(viewTarget, ScalarMult(up_vector, delta));
	}
	if (glutKeyIsDown(GLUT_KEY_DOWN)) {
		viewTarget = VectorSub(viewTarget, ScalarMult(up_vector, delta));
	}
}

void display(void)
{
	printError("pre display");

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLuint program;
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &program);

	// Send time in seconds as uniform
	GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	glUniform1f(glGetUniformLocation(program, "time"), t);

	// Send view matrix
	const vec3 up_vector = {0, 1, 0};
	mat4 viewMatrix = lookAtv(viewPos, viewTarget, up_vector);
	glUniformMatrix4fv(glGetUniformLocation(program, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);

	float delta = (t - previousT);
	previousT = t;
	handleFlyingControls(up_vector, delta);

	// Draw all the models in order
	GLuint modelMatAttr = glGetUniformLocation(program, "modelMatrix");
	GLuint useTexAttr = glGetUniformLocation(program, "useTexture");
	GLuint isSkyboxAttr = glGetUniformLocation(program, "isSkybox");

	glUniform1i(useTexAttr, 1); // Skybox and ground have textures
	// Draw the skybox
	glUniform1i(isSkyboxAttr, 1);
	glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
	setModelMatrix(modelMatAttr, Rx(0.0));
	drawModelWithTex(&skybox, skyboxTex);
	glEnable(GL_DEPTH_TEST);
	glUniform1i(isSkyboxAttr, 0);

	// Draw the ground
	setModelMatrix(modelMatAttr, T(0.0f, -10.0f, 0.0f));
	glBindTexture(GL_TEXTURE_2D, grassTex);
	glBindVertexArray(groundVAO); // Select VAO
	glDrawArrays(GL_TRIANGLES, 0, groundNumVertices);
        glEnable(GL_CULL_FACE);

	glUniform1i(useTexAttr, 0); // Windmill has no texture
	mat4 windmillMatrix = T(-10.0f, -10.0f, -10.0f);
	// Mult(T(-10.0f, -10.0f, -10.0f), Rx(0.5f * sin(0.5f * t)));

	// Draw windmill blades model
	mat4 bladesMatrix = Mult(windmillMatrix,
			Mult(T(4.56, 9.23, 0.03), Rx(t)));
	setModelMatrix(modelMatAttr, bladesMatrix);
	drawModelWithTex(&windmillBlade, grassTex);
	setModelMatrix(modelMatAttr, Mult(bladesMatrix, Rx(0.5 * M_PI)));
	drawModelWithTex(&windmillBlade, grassTex);
	setModelMatrix(modelMatAttr, Mult(bladesMatrix, Rx(1.0f * M_PI)));
	drawModelWithTex(&windmillBlade, grassTex);
	setModelMatrix(modelMatAttr, Mult(bladesMatrix, Rx(1.5f * M_PI)));
	drawModelWithTex(&windmillBlade, grassTex);

	// Draw windmill without blades
	setModelMatrix(modelMatAttr, windmillMatrix);
	drawModelWithTex(&windmillBalcony, grassTex);
	drawModelWithTex(&windmillRoof, grassTex);
	drawModelWithTex(&windmillWalls, grassTex);

	// Draw the teapot
	setModelMatrix(modelMatAttr, T(1.0f, 1.0f, 1.0f));
	drawModelWithTex(&teapot, concTex);

	// Draw the bunny
	setModelMatrix(modelMatAttr, Mult(T(-1.0f, -7.0f, 1.0f), S(5.0f, 5.0f, 5.0f)));
	drawModelWithTex(&bunny, dirtTex);

	printError("display");
	glutSwapBuffers();
}

void OnTimer(int value)
{
    glutPostRedisplay();
    glutTimerFunc(20, &OnTimer, value);
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 2);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(WIN_WIDTH, WIN_WIDTH);
	glutCreateWindow ("3: Skybox and ground");
	glutDisplayFunc(display);
	init();
	glutTimerFunc(20, OnTimer, 0);
	glutPassiveMotionFunc(MouseFunc);
	glutMainLoop();
	return 0;
}
