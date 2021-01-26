// Lab 3-1.

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

// projection matrix

#define near 1.0
#define far 100.0
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

typedef struct {
	Model *model;
	GLuint vao;
} ModelAndVAO;

// vertex array object for the models
ModelAndVAO windmillBlade;
ModelAndVAO windmillRoof;
ModelAndVAO windmillBalcony;
ModelAndVAO windmillWalls;

// Textures
GLuint squaresTex;
GLuint carTex;
GLuint dandelionTex;
GLuint dirtTex;
GLuint grassTex;

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
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	// Load and compile shader
	program = loadShaders("lab3-1.vert", "lab3-1.frag");
	printError("init shader");

	// Load models and upload geometry to the GPU:
	loadModelAndVAO(program, "windmill/blade.obj", &windmillBlade);
	loadModelAndVAO(program, "windmill/windmill-balcony.obj", &windmillBalcony);
	loadModelAndVAO(program, "windmill/windmill-roof.obj", &windmillRoof);
	loadModelAndVAO(program, "windmill/windmill-walls.obj", &windmillWalls);

	// End of upload of geometry

	// Upload textures to GPU

	// Load textures

	LoadTGATextureSimple("rutor.tga", &squaresTex);
	LoadTGATextureSimple("bilskissred.tga", &carTex);
	LoadTGATextureSimple("maskros512.tga", &dandelionTex);
	LoadTGATextureSimple("dirt.tga", &dirtTex);
	LoadTGATextureSimple("grass.tga", &grassTex);

	// Select texture 0
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(program, "texUnit"), 0);

	// End of texture upload

	// Set projection matrix uniform
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, projectionMatrix);
	printError("init matrices");
}

// Temporary variables to adjust position of bladestime
float bladesX = 4.56;
float bladesY = 9.23;
float bladesZ = 0.03;

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
	mat4 viewMatrix = lookAt(
		10.0, 5.0, 10.0,
		0, 0, 0,
		0, 1, 0
	);
	glUniformMatrix4fv(glGetUniformLocation(program, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);

	// Draw all the models in order
	GLuint modelMatAttr = glGetUniformLocation(program, "modelMatrix");
	GLuint useTexAttr = glGetUniformLocation(program, "useTexture");

	// Alter position of blades
	float delta = t / 500.0;
	if (glutKeyIsDown('w')) {
		bladesZ += delta;
	}
	if (glutKeyIsDown('s')) {
		bladesZ -= delta;
	}
	if (glutKeyIsDown('d')) {
		bladesX += delta;
	}
	if (glutKeyIsDown('a')) {
		bladesX -= delta;
	}
	if (glutKeyIsDown('q')) {
		bladesY += delta;
	}
	if (glutKeyIsDown('e')) {
		bladesY -= delta;
	}
	// printf("%f %f %f\n", bladesX, bladesY, bladesZ);

	int animate = glutKeyIsDown(' ');
	glUniform1i(useTexAttr, 0);
	mat4 windmillMatrix = animate ?
		Mult(T(-10.0f, -10.0f, -10.0f), Rx(0.5f * sin(0.5f * t))) :
		T(-10.0f, -10.0f, -10.0f);

	// Draw windmill blades model
	mat4 bladesMatrix = animate ?
		Mult(windmillMatrix, Mult(T(bladesX, bladesY, bladesZ), Rx(t))) :
		Mult(windmillMatrix, T(bladesX, bladesY, bladesZ));
	setModelMatrix(modelMatAttr, bladesMatrix);
	drawModelWithTex(&windmillBlade, squaresTex);
	setModelMatrix(modelMatAttr, Mult(bladesMatrix, Rx(0.5 * M_PI)));
	drawModelWithTex(&windmillBlade, squaresTex);
	setModelMatrix(modelMatAttr, Mult(bladesMatrix, Rx(1.0f * M_PI)));
	drawModelWithTex(&windmillBlade, squaresTex);
	setModelMatrix(modelMatAttr, Mult(bladesMatrix, Rx(1.5f * M_PI)));
	drawModelWithTex(&windmillBlade, squaresTex);

	// Draw windmill without blades
	setModelMatrix(modelMatAttr, windmillMatrix);
	drawModelWithTex(&windmillBalcony, squaresTex);
	drawModelWithTex(&windmillRoof, squaresTex);
	drawModelWithTex(&windmillWalls, squaresTex);

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
	glutInitWindowSize(1000, 1000);
	glutCreateWindow ("7: bunny, windmill and car");
	glutDisplayFunc(display);
	init();
	glutTimerFunc(20, &OnTimer, 0);
	glutMainLoop();
	return 0;
}
