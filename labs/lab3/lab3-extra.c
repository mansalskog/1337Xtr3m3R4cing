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

// light sources

vec3 lightSourcesColorsArr[] = {
    {1.0f, 0.0f, 0.0f}, // Red light
    {0.0f, 1.0f, 0.0f}, // Green light
    {0.0f, 0.0f, 1.0f}, // Blue light
    {1.0f, 1.0f, 1.0f} // White light
};

GLint isDirectional[] = {0, 0, 1, 1};

vec3 lightSourcesDirectionsPositions[] = {
    {10.0f, 5.0f, 0.0f}, // Red light, positional
    {0.0f, 5.0f, 10.0f}, // Green light, positional
    {-1.0f, 0.0f, 0.0f}, // Blue light along X
    {0.0f, 0.0f, -1.0f} // White light along Z
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
const GLfloat groundNormals[] = {
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,

	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
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

// Loaded models and some attributes (texture id, specular exponent)
typedef struct {
    Model *model;
    GLuint vao;
	GLuint textureID;
    GLfloat specularExpt;
} ModelAndVAO;

ModelAndVAO windmillBlade;
ModelAndVAO windmillRoof;
ModelAndVAO windmillBalcony;
ModelAndVAO windmillWalls;
ModelAndVAO teapot;
ModelAndVAO bunny;
ModelAndVAO skybox;

// Textures
GLuint skyboxTex;
GLuint dirtTex;
GLuint grassTex;
GLuint concreteTex;

// Specular exponents
const GLfloat windmillSpecular = 200.0f;
const GLfloat teapotSpecular = 100.0f;
const GLfloat bunnySpecular = 50.0f;
const GLfloat groundSpecular = 0.0f;

void loadModelAndVAO(GLuint program,
					 const char *filename,
					 ModelAndVAO *m,
					 GLuint texID,
					 GLfloat specExpt) {
    m->model = LoadModel(filename);
    if (!m) {
        fprintf(stderr, "Model is NULL!\n");
        exit(1);
    }

	// Save properties
	m->textureID = texID;
	m->specularExpt = specExpt;

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

void drawModel(GLuint program, const ModelAndVAO *m) {
	// Set specular exponent
	GLuint specularExptAttr = glGetUniformLocation(program, "specularExpt");
	glUniform1f(specularExptAttr, m->specularExpt);
	// Select the texture
	glBindTexture(GL_TEXTURE_2D, m->textureID);
	// Use texture only if model has texture coordinates
	GLuint useTexAttr = glGetUniformLocation(program, "useTexture");
	glUniform1i(useTexAttr, m->model->texCoordArray != NULL);
	// Select VAO and draw it
	glBindVertexArray(m->vao);
    glDrawElements(GL_TRIANGLES, m->model->numIndices, GL_UNSIGNED_INT, 0L);
}

void setModelMatrix(GLuint program, mat4 matrix) {
	GLuint modelMatAttr = glGetUniformLocation(program, "modelMatrix");
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
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    printError("GL inits");

    // Load and compile shader
    program = loadShaders("lab3-extra.vert", "lab3-extra.frag");
    printError("init shader");

	// Upload textures to GPU

    // Load textures

    LoadTGATextureSimple("dirt.tga", &dirtTex);
    LoadTGATextureSimple("grass.tga", &grassTex);
    LoadTGATextureSimple("SkyBox512.tga", &skyboxTex);
	LoadTGATextureSimple("conc.tga", &concreteTex);

    // Select texture 0
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(program, "texUnit"), 0);

    // End of texture upload

    // Load models and upload geometry to the GPU:
    loadModelAndVAO(program, "windmill/blade.obj", &windmillBlade, dirtTex, windmillSpecular);
    loadModelAndVAO(program, "windmill/windmill-balcony.obj", &windmillBalcony, dirtTex, windmillSpecular);
    loadModelAndVAO(program, "windmill/windmill-roof.obj", &windmillRoof, dirtTex, windmillSpecular);
    loadModelAndVAO(program, "windmill/windmill-walls.obj", &windmillWalls, dirtTex, windmillSpecular);

	loadModelAndVAO(program, "teapot.obj", &teapot, concreteTex, teapotSpecular);
	loadModelAndVAO(program, "bunnyplus.obj", &bunny, dirtTex, bunnySpecular);
    loadModelAndVAO(program, "skybox.obj", &skybox, skyboxTex, 0.0f);

    // Vertex array object for ground
    glGenVertexArrays(1, &groundVAO);
    GLuint groundVBO, groundNormalBO, groundTexCoordBO;
    glGenBuffers(1, &groundVBO);
	glGenBuffers(1, &groundNormalBO);
    glGenBuffers(1, &groundTexCoordBO);
    glBindVertexArray(groundVAO);

    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, groundNumVertices * 3 * sizeof(GLfloat), groundVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(glGetAttribLocation(program, "inPosition"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "inPosition"));
    printError("init positions for ground");

	glBindBuffer(GL_ARRAY_BUFFER, groundNormalBO);
    glBufferData(GL_ARRAY_BUFFER, groundNumVertices * 3 * sizeof(GLfloat), groundNormals, GL_STATIC_DRAW);
    glVertexAttribPointer(glGetAttribLocation(program, "inNormal"), 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "inNormal"));
    printError("init normals for ground");

    glBindBuffer(GL_ARRAY_BUFFER, groundTexCoordBO);
    glBufferData(GL_ARRAY_BUFFER, groundNumVertices * 2 * sizeof(GLfloat), groundTexCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(glGetAttribLocation(program, "inTexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(glGetAttribLocation(program, "inTexCoord"));
    printError("init texture coords for ground");

    // End of upload of geometry

    // Set projection matrix uniform
    glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_TRUE, projectionMatrix);
    printError("init matrices");

	// Upload light sources to shader
	glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), 4, &lightSourcesDirectionsPositions[0].x);
	glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), 4, &lightSourcesColorsArr[0].x);
	glUniform1f(glGetUniformLocation(program, "specularExponent"), 123.0f);
	glUniform1iv(glGetUniformLocation(program, "isDirectional"), 4, isDirectional);
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

    GLuint p;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *) &p);

    // Send time in seconds as uniform
    GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    glUniform1f(glGetUniformLocation(p, "time"), t);

    // Send view matrix
    const vec3 up_vector = {0, 1, 0};
    mat4 viewMatrix = lookAtv(viewPos, viewTarget, up_vector);
    glUniformMatrix4fv(glGetUniformLocation(p, "viewMatrix"), 1, GL_TRUE, viewMatrix.m);

    float delta = (t - previousT);
	previousT = t;
    handleFlyingControls(up_vector, delta);

    // Draw all the models in order

	// Draw the skybox
    GLuint isSkyboxAttr = glGetUniformLocation(p, "isSkybox");
    glUniform1i(isSkyboxAttr, 1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    setModelMatrix(p, Rx(0.0));
    drawModel(p, &skybox);
    glEnable(GL_DEPTH_TEST);
    glUniform1i(isSkyboxAttr, 0);

    // Draw the ground
    setModelMatrix(p, T(0.0f, -10.0f, 0.0f));
    glBindTexture(GL_TEXTURE_2D, grassTex);
    glBindVertexArray(groundVAO); // Select VAO
    glDrawArrays(GL_TRIANGLES, 0, groundNumVertices);
    glEnable(GL_CULL_FACE);

	// Draw the windmill
    mat4 windmillMatrix = T(-10.0f, -10.0f, -10.0f);

    // Draw windmill blades model
    mat4 bladesMatrix = Mult(windmillMatrix,
                             Mult(T(4.56, 9.23, 0.03), Rx(t)));
    setModelMatrix(p, bladesMatrix);
    drawModel(p, &windmillBlade);
    setModelMatrix(p, Mult(bladesMatrix, Rx(0.5 * M_PI)));
    drawModel(p, &windmillBlade);
    setModelMatrix(p, Mult(bladesMatrix, Rx(1.0f * M_PI)));
    drawModel(p, &windmillBlade);
    setModelMatrix(p, Mult(bladesMatrix, Rx(1.5f * M_PI)));
    drawModel(p, &windmillBlade);

    // Draw windmill without blades
    setModelMatrix(p, windmillMatrix);
    drawModel(p, &windmillBalcony);
    drawModel(p, &windmillRoof);
    drawModel(p, &windmillWalls);

	// Draw the teapot
	setModelMatrix(p, T(1.0f, 1.0f, 1.0f));
	drawModel(p, &teapot);

	// Draw the bunny
	setModelMatrix(p, Mult(T(-1.0f, -7.0f, 1.0f), S(5.0f, 5.0f, 5.0f)));
	drawModel(p, &bunny);

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
    glutCreateWindow ("4: Phong lighting");
    glutDisplayFunc(display);
    init();
    glutTimerFunc(20, OnTimer, 0);
    glutPassiveMotionFunc(MouseFunc);
    glutMainLoop();
    return 0;
}
