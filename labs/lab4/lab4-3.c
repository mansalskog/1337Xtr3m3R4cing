// Lab 4, terrain generation

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
	// Linking hint for Lightweight IDE
	// uses framework Cocoa
#endif
#include "MicroGlut.h"
#include "GL_utilities.h"
#include "VectorUtils3.h"
#include "LittleOBJLoader.h"
#include "LoadTGA.h"

const int WIN_WIDTH = 600;
const int WIN_HEIGHT = 600;

const float HM_SCALE = 2.0;

int lightCount = 3;
GLfloat lightSourcesDirPosArr[] = {
	-1.0, -1.0, -1.0,
	128.0, 50.0, 128.0,
	150.0, 150.0, 0.0,
};
GLfloat lightSourcesColorArr[] = {
	1.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 1.0,
};
int isDirectional[] = {
	1,
	0,
	0,
};

mat4 projectionMatrix;

float hmValueAt(const TextureData *tex, int x0, int z0) {
	return tex->imageData[(x0 + z0 * tex->width) * (tex->bpp/8)] / 7.0;
}

vec3 normalForTriangle(
		const TextureData *tex,
		int x0, int z0,
		int x1, int z1,
		int x2, int z2) {
	vec3 a = SetVector(x0 * HM_SCALE, hmValueAt(tex, x0, z0), z0 * HM_SCALE);
	vec3 b = SetVector(x1 * HM_SCALE, hmValueAt(tex, x1, z1), z1 * HM_SCALE);
	vec3 c = SetVector(x2 * HM_SCALE, hmValueAt(tex, x2, z2), z2 * HM_SCALE);
	return Normalize(CrossProduct(VectorSub(b, a), VectorSub(c, a)));
}

Model* GenerateTerrain(TextureData *tex)
{
	int vertexCount = tex->width * tex->height;
	int triangleCount = (tex->width-1) * (tex->height-1) * 2;
	int x, z;

	vec3 *vertexArray = malloc(sizeof(vec3) * vertexCount);
	vec3 *normalArray = malloc(sizeof(vec3) * vertexCount);
	vec2 *texCoordArray = malloc(sizeof(vec2) * vertexCount);
	GLuint *indexArray = malloc(sizeof(GLuint) * triangleCount * 3);

	printf("bpp %d\n", tex->bpp);
	for (x = 0; x < (int) tex->width; x++)
		for (z = 0; z < (int) tex->height; z++)
		{
// Vertex array. You need to scale this properly
			vertexArray[x + z * tex->width] = SetVector(
					x / 1.0,
					hmValueAt(tex, x, z),
					z / 1.0);
// Normal vectors. You need to calculate these.
			normalArray[x + z * tex->width] = SetVector(0.0, 1.0, 0.0);
// Texture coordinates. You may want to scale them.
			texCoordArray[x + z * tex->width].x = x * HM_SCALE; // (float)x / tex->width;
			texCoordArray[x + z * tex->width].y = z * HM_SCALE; // (float)z / tex->height;
		}
	for (x = 0; x < (int) tex->width-1; x++)
		for (z = 0; z < (int) tex->height-1; z++)
		{
		// Triangle 1
			indexArray[(x + z * (tex->width-1))*6 + 0] = x + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 1] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 2] = x+1 + z * tex->width;
		// Triangle 2
			indexArray[(x + z * (tex->width-1))*6 + 3] = x+1 + z * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 4] = x + (z+1) * tex->width;
			indexArray[(x + z * (tex->width-1))*6 + 5] = x+1 + (z+1) * tex->width;
		}

	for (x = 0; x < (int) tex->width; x++)
		for (z = 0; z < (int) tex->height; z++)
		{
			vec3 normal = {0.0, 0.0, 0.0};
			if (x+1 < (int) tex->width && z+1 < (int) tex->height) {
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(tex,
						x, z, x, z+1, x+1, z), 2.0));
			}
			if (x+1 < (int) tex->width && z-1 > 0) {
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(tex,
						x, z, x+1, z, x+1, z-1), 1.0));
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(tex,
						x, z, x+1, z-1, x, z-1), 1.0));
			}
			if (x-1 > 0 && z-1 > 0) {
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(tex,
						x, z, x, z-1, x-1, z), 2.0));
			}
			if (x-1 >= 0 && z+1 < (int) tex->height) {
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(tex,
						x, z, x-1, z, x-1, z+1), 1.0));
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(tex,
						x, z, x-1, z+1, x, z+1), 1.0));
			}
			/*
			vec3 normal = {0.0, 0.0, 0.0};
			if (0 <= x-1 && x+1 < (int) tex->width && 0 <= z-1 && z+1 < (int) tex->height)  {
				vec3 a = SetVector((x-1) * HM_SCALE, hmValueAt(tex, x-1, z-1), (z-1) * HM_SCALE);
				vec3 b = SetVector((x+1) * HM_SCALE, hmValueAt(tex, x+1, z), z * HM_SCALE);
				vec3 c = SetVector(x * HM_SCALE, hmValueAt(tex, x, z+1), (z+1) * HM_SCALE);
				normal = CrossProduct(VectorSub(c, a), VectorSub(b, a));
				//normal = ScalarMult(normalForTriangle(tex, x, z, x, z+1, x+1, z), -1.0);
			}
			*/
			normalArray[x + z * tex->width] = Normalize(normal);
		}

	// End of terrain generation

	// Create Model and upload to GPU:

	Model* model = LoadDataToModel(
			vertexArray,
			normalArray,
			texCoordArray,
			NULL,
			indexArray,
			vertexCount,
			triangleCount*3);

	return model;
}


// vertex array object
Model *m, *m2, *tm;
// Reference to shader program
GLuint program;
GLuint tex1, tex2;
TextureData ttex; // terrain

void init(void)
{
	// GL inits
	glClearColor(0.2,0.2,0.5,0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, 500.0);

	// Load and compile shader
	program = loadShaders("lab4-3.vert", "lab4-3.frag");
	glUseProgram(program);
	printError("init shader");

	glUniformMatrix4fv(glGetUniformLocation(program, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
	glUniform1i(glGetUniformLocation(program, "tex"), 0); // Texture unit 0
	LoadTGATextureSimple("conc.tga", &tex1);

// Load terrain data

	LoadTGATextureData("fft-terrain.tga", &ttex);
	tm = GenerateTerrain(&ttex);
	printError("init terrain");

	// Setup light sources
	glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), lightCount, lightSourcesDirPosArr);
	glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), lightCount, lightSourcesColorArr);
	glUniform1iv(glGetUniformLocation(program, "isDirectional"), lightCount, isDirectional);

}

const vec3 up_vector = {0.0, 1.0, 0.0};
vec3 view_target = {2, 0, 2};
vec3 view_pos = {0, 5, 8};

void display(void)
{
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 total, modelView, camMatrix;

	printError("pre display");

	glUseProgram(program);

	// Handle time
	GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	glUniform1f(glGetUniformLocation(program, "time"), t);

	// Build matrix

	camMatrix = lookAtv(view_pos, view_target, up_vector);
	modelView = IdentityMatrix();
	total = Mult(camMatrix, modelView);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, total.m);

	glBindTexture(GL_TEXTURE_2D, tex1);		// Bind Our Texture tex1
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");

	printError("display 2");

	glutSwapBuffers();
}


void handleFlyingControls(float delta) {
	const float MOVE_SPEED = 50.0f;
	const float LOOK_SPEED = 200.0f;
	// Fly around in the world, update view matrix for next frame
	const vec3 forward_vector = Normalize(VectorSub(view_target, view_pos));
	// cross product of two unit vectors is also unit
	const vec3 right_vector = Normalize(CrossProduct(forward_vector, up_vector));
	// Forward and backward
	if (glutKeyIsDown('w')) {
		view_pos = VectorAdd(view_pos, ScalarMult(forward_vector, delta * MOVE_SPEED));
	}
	if (glutKeyIsDown('s')) {
		view_pos = VectorSub(view_pos, ScalarMult(forward_vector, delta * MOVE_SPEED));
	}
	// Strafing left and right
	if (glutKeyIsDown('d')) {
		view_pos = VectorAdd(view_pos, ScalarMult(right_vector, delta * MOVE_SPEED));
	}
	if (glutKeyIsDown('a')) {
		view_pos = VectorSub(view_pos, ScalarMult(right_vector, delta * MOVE_SPEED));
	}
	// Up and down
	if (glutKeyIsDown(' ')) {
		view_pos = VectorAdd(view_pos, ScalarMult(up_vector, delta * MOVE_SPEED));
	}
	if (glutKeyIsDown('c')) {
		view_pos = VectorSub(view_pos, ScalarMult(up_vector, delta * MOVE_SPEED));
	}
	// Put view target 100 units in front of view pos
	view_target = VectorAdd(view_pos, ScalarMult(forward_vector, 100.0f));
	// Turning
	if (glutKeyIsDown(GLUT_KEY_RIGHT)) {
		view_target = VectorAdd(view_target, ScalarMult(right_vector, delta * LOOK_SPEED));
	}
	if (glutKeyIsDown(GLUT_KEY_LEFT)) {
		view_target = VectorSub(view_target, ScalarMult(right_vector, delta * LOOK_SPEED));
	}
	if (glutKeyIsDown(GLUT_KEY_UP)) {
		view_target = VectorAdd(view_target, ScalarMult(up_vector, delta * LOOK_SPEED));
	}
	if (glutKeyIsDown(GLUT_KEY_DOWN)) {
		view_target = VectorSub(view_target, ScalarMult(up_vector, delta * LOOK_SPEED));
	}
}

void handleMouseLook(float delta, int dx, int dy) {
	const int MIN_PIXELS = 5;
	const float LOOK_SPEED = 150.0f;
	const vec3 forward_vector = Normalize(VectorSub(view_target, view_pos));
	const vec3 right_vector = Normalize(CrossProduct(forward_vector, up_vector));
	// Mouse look
	if (dx > MIN_PIXELS) {
		view_target = VectorAdd(view_target, ScalarMult(right_vector, delta * LOOK_SPEED));
	}
	if (dx < -MIN_PIXELS) {
		view_target = VectorSub(view_target, ScalarMult(right_vector, delta * LOOK_SPEED));
	}
	if (dy < -MIN_PIXELS) {
		view_target = VectorAdd(view_target, ScalarMult(up_vector, delta * LOOK_SPEED));
	}
	if (dy > MIN_PIXELS) {
		view_target = VectorSub(view_target, ScalarMult(up_vector, delta * LOOK_SPEED));
	}
}

GLfloat prev_t = 0.0;
int paused = 0;

void timer(int i)
{
	glutTimerFunc(20, &timer, i);
    GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	handleFlyingControls(t - prev_t);
	prev_t = t;
	glutPostRedisplay();
}

void mouse(int x, int y)
{
	if (!paused) {
		GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
		handleMouseLook(t - prev_t, x - WIN_WIDTH / 2, y - WIN_HEIGHT / 2);
		glutWarpPointer(WIN_WIDTH / 2, WIN_HEIGHT / 2);
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == GLUT_KEY_ESC) {
		paused = !paused;
		if (paused) {
			glutShowCursor();
		} else {
			glutHideCursor();
		}
	}
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	glutCreateWindow("TSBK07 Lab 4");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	init();
	glutTimerFunc(20, &timer, 0);

	glutHideCursor();
	glutPassiveMotionFunc(mouse);

	glutMainLoop();
	exit(0);
}
