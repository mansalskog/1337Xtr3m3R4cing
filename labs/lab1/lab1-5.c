// Lab 1-2.
// This is the same as the first simple example in the course book,
// but with a few error checks.
// Remember to copy your file to a new on appropriate places during the lab so you keep old results.
// Note that the files "lab1-2.frag", "lab1-2.vert" are required.

// Should work as is on Linux and Mac. MS Windows needs GLEW or glee.
// See separate Visual Studio version of my demos.
#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	// Linking hint for Lightweight IDE
	// uses framework Cocoa
#endif
#include "MicroGlut.h"
#include "GL_utilities.h"
#include <math.h>

// Globals
// Data would normally be read from files

GLfloat vertices[] =
{
    -0.5f,0.5f,0.0f,
    0.5f,0.5f,0.0f,
    0.7f,0.7f,0.9f,

    -0.5f,0.5f,0.0f,
    -0.5f,-0.5f,0.0f,
    0.0f,0.0f,0.5f,

	-0.5f,-0.5f,0.0f,
	-0.5f,0.5f,0.0f,
	0.5f,-0.5f,0.0f,

    0.5f,0.5f,0.0f,
    -0.5f,0.5f,0.0f,
	0.5f,-0.5f,0.0f,

    -0.5f,0.5f,0.0f,
    0.5f,-0.5f,0.0f,
    0.0f,0.0f,0.5f,

    0.5f,-0.5f,0.0f,
    0.5f,0.5f,0.0f,
    0.0f,0.0f,0.5f
};

GLfloat verticesColor[] =
{
	0.2f,0.2f,0.0f,
	0.0f,0.5f,0.8f,
	0.5f,0.0f,0.6f,

    0.4f,0.5f,0.1f,
	0.0f,0.3f,0.9f,
	0.5f,0.3f,0.4f,

    0.2f,0.0f,0.0f,
	0.7f,0.5f,0.8f,
	0.5f,0.0f,0.0f,

    0.2f,0.1f,0.4f,
	0.1f,0.2f,0.5f,
	0.9f,0.9f,0.2f,

    0.2f,0.8f,0.4f,
	0.6f,0.7f,0.2f,
	0.9f,0.0f,0.9f,

    0.6f,0.5f,0.6f,
	0.6f,0.1f,0.8f,
	0.4f,0.7f,0.3f,
};


GLfloat myMatrix[] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void OnTimer(int value)
{
    glutPostRedisplay();
    glutTimerFunc(20, &OnTimer, value);
}

// vertex array object
unsigned int vertexArrayObjID;

void init(void)
{
	// vertex buffer object, used for uploading the geometry
	unsigned int vertexBufferObjID;
    unsigned int vertexBufferObjNames;
	// Reference to shader program
	GLuint program;

	dumpInfo();

	// GL inits
	glClearColor(0.2,0.2,0.5,0);
	// glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
	printError("GL inits");

	// Load and compile shader
	program = loadShaders("lab1-5.vert", "lab1-5.frag");
	printError("init shader");

	// Upload geometry to the GPU:


	// Allocate and activate Vertex Array Object
	glGenVertexArrays(1, &vertexArrayObjID);
	glBindVertexArray(vertexArrayObjID);
	// Allocate Vertex Buffer Objects
	glGenBuffers(1, &vertexBufferObjID);
    // VBO for vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID);

	glBufferData(GL_ARRAY_BUFFER, 18*3*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "in_Position"),
                          3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_Position"));


    // For Color shading
    glGenBuffers(1, &vertexBufferObjNames);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjNames);

	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesColor), verticesColor, GL_STATIC_DRAW);
	glVertexAttribPointer(glGetAttribLocation(program, "in_Color"),
                          3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(glGetAttribLocation(program, "in_Color"));



	// End of upload of geometry

	printError("init arrays");
}


void display(void)
{

	printError("pre display");
    GLuint program;
    GLfloat t = (GLfloat)glutGet(GLUT_ELAPSED_TIME);

    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &program);

    // time varying animation
    t = t/1000.0f;
    myMatrix[0] = cos(t);
    myMatrix[1] = sin(t/100.0f);
    myMatrix[4] = -sin(t);
    myMatrix[5] = cos(t);

    //Clear Z-Buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniformMatrix4fv(glGetUniformLocation(program, "myMatrix"), 1,
                       GL_TRUE, myMatrix);
	glBindVertexArray(vertexArrayObjID);	// Select VAO
	glDrawArrays(GL_TRIANGLES, 0, 18);	// draw object

	printError("display");

	glutSwapBuffers();
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 2);
	glutCreateWindow ("GL3 white triangle example");
	glutDisplayFunc(display);
    glutTimerFunc(20, &OnTimer, 0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	init ();
	glutMainLoop();
	return 0;
}
