// Lab 2-2.

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
#define far 30.0
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

// vertex array object for the models
GLuint bunnyVAO;
GLuint carVAO;
GLuint windmillBlade;
GLuint windmillRoof;
GLuint windmillBalcony;
GLuint windmillWalls;

// The models themselves
Model *bunnyModel;
Model *carModel;

void putModelIntoVAO(GLuint program, Model *m, GLuint *vertexArrayObjID) {
	if (!m) {
		fprintf(stderr, "Model is NULL!\n");
		exit(1);
	}

	// vertex buffer objects
	GLuint vertexBufferObjID;
	GLuint indexBufferObjID;
	GLuint normalBufferObjID;
	GLuint texCoordBufferObjID;

	glGenVertexArrays(1, vertexArrayObjID);
	glGenBuffers(1, &vertexBufferObjID);
	glGenBuffers(1, &indexBufferObjID);
	glGenBuffers(1, &normalBufferObjID);
	glGenBuffers(1, &texCoordBufferObjID);
	glBindVertexArray(*vertexArrayObjID);

	// VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, m->numVertices * 3 * sizeof(GLfloat), m->vertexArray, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "inPosition"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "inPosition"));
	printError("init positions");

	// VBO for normal data
	if (m->normalArray != NULL) {
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferObjID);
		glBufferData(GL_ARRAY_BUFFER, m->numVertices * 3 * sizeof(GLfloat), m->normalArray, GL_STATIC_DRAW);
		glVertexAttribPointer(glGetAttribLocation(program, "inNormal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(glGetAttribLocation(program, "inNormal"));
		printError("init normals");
	}

	// VBO for texture coordinates
	if (m->texCoordArray != NULL)
	{
		glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferObjID);
		glBufferData(GL_ARRAY_BUFFER, m->numVertices * 2 * sizeof(GLfloat), m->texCoordArray, GL_STATIC_DRAW);
		glVertexAttribPointer(glGetAttribLocation(program, "inTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(glGetAttribLocation(program, "inTexCoord"));
		printError("init texture coords");
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObjID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->numIndices * sizeof(GLuint), m->indexArray, GL_STATIC_DRAW);
	printError("init indices");
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
	program = loadShaders("lab2-7.vert", "lab2-7.frag");
	printError("init shader");

	// Load models and upload geometry to the GPU:
	bunnyModel = LoadModel("bunnyplus.obj");
	putModelIntoVAO(program, bunnyModel, &bunnyVAO);

	carModel = LoadModel("bilskiss.obj");
	putModelIntoVAO(program, carModel, &carVAO);

	// End of upload of geometry

	// Upload textures to GPU

	GLuint bunnyTexture;
	LoadTGATextureSimple("maskros512.tga", &bunnyTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bunnyTexture);

	GLuint carTexture;
	LoadTGATextureSimple("bilskissred.tga", &carTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, carTexture);

	// End of texture upload

	// Set projection matrix uniform
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, projectionMatrix);
	printError("init matrices");
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
	mat4 viewMatrix = lookAt(
		2.0, 0.5, 3.0,
		0, 0, 0,
		0, 1, 0
	);
	glUniformMatrix4fv(glGetUniformLocation(program, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);

	// Draw all the models in order
	mat4 rotation, translation, modelMatrix;
	GLuint texUnitAttr = glGetUniformLocation(program, "texUnit");
	GLuint modelMatrixAttr = glGetUniformLocation(program, "modelMatrix");

	// Draw bunny model
	rotation = Ry(t);
	translation = T(1.0f, 0.0f, 0.0f);
	modelMatrix = Mult(translation, rotation);
	glUniformMatrix4fv(modelMatrixAttr, 1, GL_TRUE, modelMatrix.m);

	glUniform1i(texUnitAttr, 0); // Select texture 0
	glBindVertexArray(bunnyVAO); // Select VAO
	glDrawElements(GL_TRIANGLES, bunnyModel->numIndices, GL_UNSIGNED_INT, 0L); // draw object

	// Draw car model
	rotation = Ry(t);
	translation = T(-1.0f, 0.0f, 0.0f);
	modelMatrix = Mult(translation, rotation);
	glUniformMatrix4fv(modelMatrixAttr, 1, GL_TRUE, modelMatrix.m);

	glUniform1i(texUnitAttr, 1); // Select texture 1
	glBindVertexArray(carVAO); // Select VAO
	glDrawElements(GL_TRIANGLES, carModel->numIndices, GL_UNSIGNED_INT, 0L); // draw object

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
	glutCreateWindow ("5: bunny with diffuse shading");
	glutDisplayFunc(display);
	init();
	glutTimerFunc(20, &OnTimer, 0);
	glutMainLoop();
	return 0;
}
