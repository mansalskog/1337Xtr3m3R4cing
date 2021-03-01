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

const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 800;

const float TILE_WIDTH_X = 3.0;
const float TILE_WIDTH_Z = 3.0;
const float TILE_HEIGHT_Y = 0.5;

int lightCount = 2;
GLfloat lightSourcesDirPosArr[] = {
	-1.0, -1.0, -1.0,
	10.0, 10.0, 10.0,
};
GLfloat lightSourcesColorArr[] = {
	1.0, 1.0, 1.0,
	1.0, 1.0, 1.0,
};
int isDirectional[] = {1, 0};

mat4 projectionMatrix;

vec3 normalForTriangle(
		const vec3 *vertexArray,
		int width,
		int x0, int z0,
		int x1, int z1,
		int x2, int z2) {
	vec3 a = vertexArray[x0 + z0 * width];
	vec3 b = vertexArray[x1 + z1 * width];
	vec3 c = vertexArray[x2 + z2 * width];
	return Normalize(CrossProduct(VectorSub(b, a), VectorSub(c, a)));
}

float pyth(float dx, float dy) {
	return sqrt(dx*dx + dy*dy);
}

float lerp(float h0, float h1, float t) {
	return h0 * t + (1 - t) * h1;
}

float hmValueAt(const TextureData *tex, int x0, int z0) {
	return tex->imageData[(x0 + z0 * tex->width) * (tex->bpp/8)] * TILE_HEIGHT_Y;
}

float hmHeightAt(const TextureData *tex, float x, float z) {
	int x0 = (int) floor(x / TILE_WIDTH_X);
	int z0 = (int) floor(z / TILE_WIDTH_Z);
	float dx = x / TILE_WIDTH_X - (float) x0;
	float dz = z / TILE_WIDTH_Z - (float) z0;

	float h00 = hmValueAt(tex, x0, z0);
	float h01 = hmValueAt(tex, x0, z0+1);
	float h10 = hmValueAt(tex, x0+1, z0);
	float h11 = hmValueAt(tex, x0+1, z0+1);

	vec3 v00 = SetVector(x0 * TILE_WIDTH_X, h00, z0 * TILE_WIDTH_Z);
	vec3 v01 = SetVector(x0 * TILE_WIDTH_X, h01, (z0+1) * TILE_WIDTH_Z);
	vec3 v10 = SetVector((x0+1) * TILE_WIDTH_X, h10, z0 * TILE_WIDTH_Z);
	vec3 v11 = SetVector((x0+1) * TILE_WIDTH_X, h11, (z0+1) * TILE_WIDTH_Z);

	if (dx + dz < 1.0) {
		vec3 normal = Normalize(CrossProduct(VectorSub(v01, v00), VectorSub(v10, v00)));
		float y = (DotProduct(normal, v00) - normal.x * x - normal.z * z) / normal.y;
		return y;
	} else {
		vec3 normal = Normalize(CrossProduct(VectorSub(v01, v10), VectorSub(v11, v10)));
		float y = (DotProduct(normal, v11) - normal.x * x - normal.z * z) / normal.y;
		return y;
	}
}

vec3 hmNormalAt(const TextureData *tex, float x, float z) {
	int x0 = (int) floor(x / TILE_WIDTH_X);
	int z0 = (int) floor(z / TILE_WIDTH_Z);
	float dx = x / TILE_WIDTH_X - (float) x0;
	float dz = z / TILE_WIDTH_Z - (float) z0;

	float h00 = hmValueAt(tex, x0, z0);
	float h01 = hmValueAt(tex, x0, z0+1);
	float h10 = hmValueAt(tex, x0+1, z0);
	float h11 = hmValueAt(tex, x0+1, z0+1);

	vec3 v00 = SetVector(x0 * TILE_WIDTH_X, h00, z0 * TILE_WIDTH_Z);
	vec3 v01 = SetVector(x0 * TILE_WIDTH_X, h01, (z0+1) * TILE_WIDTH_Z);
	vec3 v10 = SetVector((x0+1) * TILE_WIDTH_X, h10, z0 * TILE_WIDTH_Z);
	vec3 v11 = SetVector((x0+1) * TILE_WIDTH_X, h11, (z0+1) * TILE_WIDTH_Z);

	vec3 normal;
	if (dx + dz < 1.0) {
		normal = Normalize(CrossProduct(VectorSub(v01, v00), VectorSub(v10, v00)));
	} else {
		normal = Normalize(CrossProduct(VectorSub(v01, v10), VectorSub(v11, v10)));
	}
	return normal;
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
					x * TILE_WIDTH_X,
					hmValueAt(tex, x, z),
					z * TILE_WIDTH_Z);
// Normal vectors. You need to calculate these.
			normalArray[x + z * tex->width] = SetVector(0.0, 1.0, 0.0);
// Texture coordinates. You may want to scale them.
			texCoordArray[x + z * tex->width].x = x; // (float)x / tex->width;
			texCoordArray[x + z * tex->width].y = z; // (float)z / tex->height;
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
			float totAng = 0.0;
			vec3 normal = {0.0, 0.0, 0.0};
			if (x+1 < (int) tex->width && z+1 < (int) tex->height) {
				totAng += 90.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, tex->width,
						x, z, x+1, z, x, z+1), 90.0));
			}
			if (x-1 >= 0 && z+1 < (int) tex->height) {
				totAng += 45.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, tex->width,
						x, z, x, z+1, x-1, z+1), 45.0));
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, tex->width,
						x, z, x-1, z+1, x-1, z), 45.0));
			}
			if (x-1 > 0 && z-1 > 0) {
				totAng += 90.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, tex->width,
						x, z, x-1, z, x, z-1), 90.0));
			}
			if (x+1 < (int) tex->width && z-1 > 0) {
				totAng += 90.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, tex->width,
						x, z, x, z-1, x+1, z-1), 45.0));
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, tex->width,
						x, z, x+1, z-1, x+1, z), 45.0));
			}
			normalArray[x + z * tex->width] = ScalarMult(normal, totAng);
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
GLuint maskros, concrete, dirt, grass;
TextureData ttex; // terrain
// Test models
Model *sphere;
Model *octagon;

void init(void)
{
	// GL inits
	glClearColor(0.2,0.2,0.5,0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, 500.0);

	// Load and compile shader
	program = loadShaders("lab4-6.vert", "lab4-6.frag");
	glUseProgram(program);
	printError("init shader");

	glUniformMatrix4fv(glGetUniformLocation(program, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
	glUniform1i(glGetUniformLocation(program, "tex0"), 0); // Texture unit 0
	glUniform1i(glGetUniformLocation(program, "tex1"), 1); // Texture unit 1
	glUniform1i(glGetUniformLocation(program, "tex2"), 2); // Texture unit 2
	LoadTGATextureSimple("maskros512.tga", &maskros);
	LoadTGATextureSimple("conc.tga", &concrete);
	LoadTGATextureSimple("dirt.tga", &dirt);
	LoadTGATextureSimple("grass.tga", &grass);

	// Load terrain data
	LoadTGATextureData("fft-terrain.tga", &ttex);
	tm = GenerateTerrain(&ttex);
	printError("init terrain");

	// Load models
	sphere = LoadModel("groundsphere.obj");
	octagon = LoadModel("octagon.obj");

	// Setup light sources
	glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), lightCount, lightSourcesDirPosArr);
	glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), lightCount, lightSourcesColorArr);
	glUniform1iv(glGetUniformLocation(program, "isDirectional"), lightCount, isDirectional);

}

const vec3 up_vector = {0.0, 1.0, 0.0};
vec3 view_target = {2, 0, 2};
vec3 view_pos = {80, 5, 108};

void display(void)
{
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 total, modelView, camMatrix;
	float x, y, z;

	printError("pre display");

	glUseProgram(program);

	// Set view matrix
	camMatrix = lookAtv(view_pos, view_target, up_vector);
	glUniformMatrix4fv(glGetUniformLocation(program, "camMatrix"), 1, GL_TRUE, camMatrix.m);

	GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	glUniform1f(glGetUniformLocation(program, "time"), t);

	modelView = IdentityMatrix();
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grass);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, dirt);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, concrete);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");

	x = 60.0 + 30.0 * cos(t / 3.0);
	z = 60.0 + 30.0 * sin(t / 3.0);
	y = hmHeightAt(&ttex, x, z);
	vec3 n = hmNormalAt(&ttex, x, z);
	vec3 v = VectorSub(SetVector(1.0, 0.0, 0.0), ScalarMult(n, n.x));
	vec3 u = Normalize(CrossProduct(n, v));
	mat4 tiltMatrix1 = {{
		v.x, v.y, v.z, 0.0,
		n.x, n.y, n.z, 0.0,
		u.x, u.y, u.z, 0.0,
		0.0, 0.0, 0.0, 1.0,
	}};
	modelView = Mult(Mult(T(x, y, z), tiltMatrix1), S(5.0, 5.0, 5.0));
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	glBindTexture(GL_TEXTURE_2D, maskros);
	DrawModel(sphere, program, "inPosition", "inNormal", "inTexCoord");

	x = 60.0 + 50.0 * pow(cos(t / 4.0), 2);
	z = 60.0 + 50.0 * pow(sin(t / 4.0), 1);
	y = hmHeightAt(&ttex, x, z);
	n = hmNormalAt(&ttex, x, z);
	v = VectorSub(SetVector(1.0, 0.0, 0.0), ScalarMult(n, n.x));
	u = Normalize(CrossProduct(n, v));
	mat4 tiltMatrix2 = {{
		v.x, v.y, v.z, 0.0,
		n.x, n.y, n.z, 0.0,
		u.x, u.y, u.z, 0.0,
		0.0, 0.0, 0.0, 1.0,
	}};
	modelView = Mult(Mult(T(x, y, z), tiltMatrix2), S(5.0, 5.0, 5.0));
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	glBindTexture(GL_TEXTURE_2D, maskros);
	DrawModel(octagon, program, "inPosition", "inNormal", "inTexCoord");

	printError("display 2");

	glutSwapBuffers();
}

float fall_speed = 0.0f;

void handleMovingControls(float delta) {
	const float MOVE_SPEED = 50.0f;
	const float LOOK_SPEED = 200.0f;
	const float FALL_ACCEL = 50.0f;
	const float JUMP_SPEED = 50.0f;
	const vec3 forward = Normalize(VectorSub(view_target, view_pos));
	const vec3 proj_forward = Normalize(SetVector(forward.x, 0, forward.z));
	const vec3 right = CrossProduct(proj_forward, up_vector);
	const vec3 proj_right = Normalize(SetVector(right.x, 0, right.z));
	// Forward and backward
	if (glutKeyIsDown('w')) {
		view_pos = VectorAdd(view_pos, ScalarMult(proj_forward, delta * MOVE_SPEED));
	}
	if (glutKeyIsDown('s')) {
		view_pos = VectorSub(view_pos, ScalarMult(proj_forward, delta * MOVE_SPEED));
	}
	// Strafing left and right
	if (glutKeyIsDown('d')) {
		view_pos = VectorAdd(view_pos, ScalarMult(proj_right, delta * MOVE_SPEED));
	}
	if (glutKeyIsDown('a')) {
		view_pos = VectorSub(view_pos, ScalarMult(proj_right, delta * MOVE_SPEED));
	}
	// Put eyes at correct height
	float ground_y = hmHeightAt(&ttex, view_pos.x, view_pos.z) + 5.0f;
	if (view_pos.y > ground_y) {
		// Falling
		fall_speed += FALL_ACCEL * delta;
		view_pos.y -= fall_speed * delta;
	} else if (glutKeyIsDown(' ')) {
		// Jumping
		fall_speed = -JUMP_SPEED;
		view_pos.y -= fall_speed * delta;
	} else {
		// Standing
		view_pos.y = ground_y;
		fall_speed = 0.0f;
	}
	// Put view target 100 units in front of view pos
	view_target = VectorAdd(view_pos, ScalarMult(forward, 100.0f));
	// Turning
	if (glutKeyIsDown(GLUT_KEY_RIGHT)) {
		view_target = VectorAdd(view_target, ScalarMult(proj_right, delta * LOOK_SPEED));
	}
	if (glutKeyIsDown(GLUT_KEY_LEFT)) {
		view_target = VectorSub(view_target, ScalarMult(proj_right, delta * LOOK_SPEED));
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
	handleMovingControls(t - prev_t);
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
