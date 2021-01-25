// Lab 1-1.
// This is the same as the first simple example in the course book,
// but with a few error checks.
// Remember to copy your file to a new on appropriate places during the lab so you keep old results.
// Note that the files "lab1-1.frag", "lab1-1.vert" are required.

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

// Globals

GLfloat zMatrix[] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

GLfloat yMatrix[] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

// vertex array object for the model
unsigned int bunnyVertexArrayObjID;

// Loaded model
Model *m;

void init(void)
{
	// vertex buffer objects, for the bunny
	unsigned int bunnyVertexBufferObjID;
	unsigned int bunnyIndexBufferObjID;
	unsigned int bunnyNormalBufferObjID;
	// Reference to shader program
	GLuint program;

	dumpInfo();

	// GL inits
	glClearColor(0.1,0.1,0.1,0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	// Load and compile shader
	program = loadShaders("lab1-6.vert", "lab1-6.frag");
	printError("init shader");

	// Load model
	m = LoadModel("bunny.obj");
	if (!m) {
		fprintf(stderr, "Could not load file");
		exit(1);
	}

	// Upload geometry to the GPU:

	glGenVertexArrays(1, &bunnyVertexArrayObjID);
	glGenBuffers(1, &bunnyVertexBufferObjID);
	glGenBuffers(1, &bunnyIndexBufferObjID);
	glGenBuffers(1, &bunnyNormalBufferObjID);
	glBindVertexArray(bunnyVertexArrayObjID);

	// VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, bunnyVertexBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, m->numVertices * 3 * sizeof(GLfloat), m->vertexArray, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "in_Position"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_Position"));
	printError("init positions");

        // VBO for normal data
	glBindBuffer(GL_ARRAY_BUFFER, bunnyNormalBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, m->numVertices * 3 * sizeof(GLfloat), m->normalArray, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "in_Normal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_Normal"));
	printError("init normals");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bunnyIndexBufferObjID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->numIndices * sizeof(GLuint), m->indexArray, GL_STATIC_DRAW);
	printError("init indices");

	// End of upload of geometry

	// Set transformation matrices
	glUniformMatrix4fv(glGetUniformLocation(program, "zMatrix"), 1, GL_TRUE, zMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "yMatrix"), 1, GL_TRUE, yMatrix);
	printError("init matrices");
}


void display(void)
{
	GLuint program;
	GLfloat t;

	printError("pre display");

	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &program);

	t = (GLfloat) glutGet(GLUT_ELAPSED_TIME);
	GLfloat a = t / 1000.0;

	// Set matrix to rotation around the z-axis
	zMatrix[0] = sin(a);
	zMatrix[1] = -cos(a);
	zMatrix[4] = cos(a);
	zMatrix[5] = sin(a);

	// Rotation around the y-axis
	yMatrix[0] = sin(a);
	yMatrix[2] = -cos(a);
	yMatrix[8] = cos(a);
	yMatrix[10] = sin(a);

	// Send matrix to the GPU
	glUniformMatrix4fv(glGetUniformLocation(program, "zMatrix"), 1, GL_TRUE, zMatrix);
	glUniformMatrix4fv(glGetUniformLocation(program, "yMatrix"), 1, GL_TRUE, yMatrix);

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(bunnyVertexArrayObjID); // Select VAO
	glDrawElements(GL_TRIANGLES, m->numIndices, GL_UNSIGNED_INT, 0L); // draw object

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
	glutCreateWindow ("6: GL3 animated bunny");
	glutDisplayFunc(display);
	init ();
	glutTimerFunc(20, &OnTimer, 0);
	glutMainLoop();
	return 0;
}
