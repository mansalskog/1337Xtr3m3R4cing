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

#include "MicroGlut.h"
#include "GL_utilities.h"

// Globals
// Data would normally be read from files
GLfloat vertices[] =
{
	-0.5f,-0.5f,0.0f,
	-0.5f,0.5f,0.0f,
	0.5f,-0.5f,0.0f
};

GLfloat colors[] = {
	0.9, 0.0, 0.0,
	0.0, 0.9, 0.0,
	0.0, 0.0, 0.9
};

GLfloat myMatrix[] = {
	1.0f, 0.0f, 0.0f, 0.5f,
	0.0f, 1.5f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

// vertex array object
unsigned int vertexArrayObjID;

void init(void)
{
	// vertex buffer object, used for uploading the geometry
	unsigned int vertexBufferObjID;
	unsigned int colorBufferObjID;
	// Reference to shader program
	GLuint program;

	dumpInfo();

	// GL inits
	glClearColor(0.1,0.1,0.1,0);
	glDisable(GL_DEPTH_TEST);
	printError("GL inits");

	// Load and compile shader
	program = loadShaders("lab1-4.vert", "lab1-4.frag");
	printError("init shader");

	// Upload geometry to the GPU:

	// Allocate and activate Vertex Array Object
	glGenVertexArrays(1, &vertexArrayObjID);
	glBindVertexArray(vertexArrayObjID);
	// Allocate Vertex Buffer Objects
	glGenBuffers(1, &vertexBufferObjID);

	// VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "in_Position"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_Position"));

	// End of upload of geometry

	// Set the transformation matrix
	glUniformMatrix4fv(glGetUniformLocation(program, "my_Matrix"), 1, GL_TRUE, myMatrix);

	// Upload colors to GPU

	// Allocate and activate VAO
	//glGenVertexArrays(1, &colorArrayObjID);
	// glBindVertexArray(colorArrayObjID);
	// Allocate VBO
	glGenBuffers(1, &colorBufferObjID);

	// VBO for color data
	glBindBuffer(GL_ARRAY_BUFFER, colorBufferObjID);
	glBufferData(GL_ARRAY_BUFFER, sizeof colors, colors, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "in_Color"), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_Color"));

	// End of upload of colors

	printError("init arrays");
}


void display(void)
{
	GLuint program;
	GLfloat t;

	printError("pre display");

	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &program);

	t = (GLfloat) glutGet(GLUT_ELAPSED_TIME);

	// Set matrix to rotation around the z-axis
	GLfloat a = t / 1000.0;
	myMatrix[0] = sin(a);
	myMatrix[1] = -cos(a);
	myMatrix[4] = cos(a);
	myMatrix[5] = sin(a);

	// Set x offset in matrix
	myMatrix[3] = sin(a);

	// Send matrix to the GPU
	glUniformMatrix4fv(glGetUniformLocation(program, "my_Matrix"), 1, GL_TRUE, myMatrix);

	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(vertexArrayObjID);	// Select VAO
	glDrawArrays(GL_TRIANGLES, 0, 3);	// draw object

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
	glutCreateWindow ("GL3 white triangle example");
	glutDisplayFunc(display);
	init ();
	glutTimerFunc(20, &OnTimer, 0);
	glutMainLoop();
	return 0;
}
