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

///// Utility functions /////

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

float interpolate(float x0, float x1, float t) {
	return x0 + (x1 - x0) * t;
}

float smoothstep(float x0, float x1, float t) {
	return x0 + (x1 - x0) * (3.0f - t * 2.0f) * t * t;
}

///// Terrain (heightmap) /////

const float TERRAIN_WIDTH_FACTOR = 15.0f;
const float TERRAIN_DEPTH_FACTOR = 15.0f;
const float TERRAIN_HEIGHT_FACTOR = 10.0f;
const int TERRAIN_WIDTH = 100;
const int TERRAIN_DEPTH = 100;

vec2 terrain_gradient(int x0, int z0) {
	float angle = 2920.f * sin(x0 * 21942.f + z0 * 171324.f + 8912.f) * cos(x0 * 23157.f * z0 * 217832.f + 9758.f);
	vec2 g = {cos(angle), sin(angle)};
	return g;
}

/* Return dot product of vector from (x0, z0) to (x, z) with "random" unit vector
 * depending only on x0 and z0.
 */
float terrain_dot_gradient(int x0, int z0, float x, float z) {
	vec2 g = terrain_gradient(x0, z0);
	float dx = x - x0;
	float dz = z - z0;
	return g.x * dx + g.y * dz;
}

float terrain_height_at(float x, float z) {
	x /= TERRAIN_WIDTH_FACTOR;
	z /= TERRAIN_DEPTH_FACTOR;

	int x0 = (int) x;
	int z0 = (int) z;
	float dx = x - x0;
	float dz = z - z0;

	float d00 = terrain_dot_gradient(x0, z0, x, z);
	float d01 = terrain_dot_gradient(x0, z0+1, x, z);
	float d10 = terrain_dot_gradient(x0+1, z0, x, z);
	float d11 = terrain_dot_gradient(x0+1, z0+1, x, z);

	return TERRAIN_HEIGHT_FACTOR * smoothstep(smoothstep(d00, d10, dx), smoothstep(d01, d11, dx), dz);
}

/*

float hmValueAt(const TextureData *tex, int x0, int z0) {
	return tex->imageData[(x0 + z0 * tex->width) * (tex->bpp/8)] * TILE_HEIGHT_Y;
}

float hmHeightAt(const TextureData *tex, float x, float z) {
	// Loop heightmap modulo width
	const float w = TILE_WIDTH_X * tex->width;
	const float h = TILE_WIDTH_Z * tex->height;
	x = fmod(fmod(x, w) + w, w);
	z = fmod(fmod(z, h) + h, h);
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

*/

vec3 hmNormalAt(float x, float z) {
	int x0 = (int) floor(x / TILE_WIDTH_X);
	int z0 = (int) floor(z / TILE_WIDTH_Z);
	float dx = x / TILE_WIDTH_X - (float) x0;
	float dz = z / TILE_WIDTH_Z - (float) z0;

	float h00 = terrain_height_at(x0, z0);
	float h01 = terrain_height_at(x0, z0+1);
	float h10 = terrain_height_at(x0+1, z0);
	float h11 = terrain_height_at(x0+1, z0+1);

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

Model* GenerateTerrain()
{
	int vertexCount = TERRAIN_WIDTH * TERRAIN_DEPTH;
	int triangleCount = (TERRAIN_WIDTH-1) * (TERRAIN_DEPTH-1) * 2;
	int x, z;

	vec3 *vertexArray = malloc(sizeof(vec3) * vertexCount);
	vec3 *normalArray = malloc(sizeof(vec3) * vertexCount);
	vec2 *texCoordArray = malloc(sizeof(vec2) * vertexCount);
	GLuint *indexArray = malloc(sizeof(GLuint) * triangleCount * 3);

	for (x = 0; x < (int) TERRAIN_WIDTH; x++)
		for (z = 0; z < (int) TERRAIN_DEPTH; z++)
		{
// Vertex array. You need to scale this properly
			vertexArray[x + z * TERRAIN_WIDTH] = SetVector(
					x * TILE_WIDTH_X,
					terrain_height_at(x, z),
					z * TILE_WIDTH_Z);
// Normal vectors. You need to calculate these.
			normalArray[x + z * TERRAIN_WIDTH] = SetVector(0.0, 1.0, 0.0);
// Texture coordinates. You may want to scale them.
			texCoordArray[x + z * TERRAIN_WIDTH].x = x; // (float)x / tex->width;
			texCoordArray[x + z * TERRAIN_WIDTH].y = z; // (float)z / tex->height;
		}
	for (x = 0; x < (int) TERRAIN_WIDTH-1; x++)
		for (z = 0; z < (int) TERRAIN_DEPTH-1; z++)
		{
		// Triangle 1
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 0] = x + z * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 1] = x + (z+1) * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 2] = x+1 + z * TERRAIN_WIDTH;
		// Triangle 2
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 3] = x+1 + z * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 4] = x + (z+1) * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 5] = x+1 + (z+1) * TERRAIN_WIDTH;
		}

	for (x = 0; x < (int) TERRAIN_WIDTH; x++)
		for (z = 0; z < (int) TERRAIN_DEPTH; z++)
		{
			float totAng = 0.0;
			vec3 normal = {0.0, 0.0, 0.0};
			if (x+1 < (int) TERRAIN_WIDTH && z+1 < (int) TERRAIN_DEPTH) {
				totAng += 90.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, TERRAIN_WIDTH,
						x, z, x+1, z, x, z+1), 90.0));
			}
			if (x-1 >= 0 && z+1 < (int) TERRAIN_DEPTH) {
				totAng += 45.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, TERRAIN_WIDTH,
						x, z, x, z+1, x-1, z+1), 45.0));
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, TERRAIN_WIDTH,
						x, z, x-1, z+1, x-1, z), 45.0));
			}
			if (x-1 > 0 && z-1 > 0) {
				totAng += 90.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, TERRAIN_WIDTH,
						x, z, x-1, z, x, z-1), 90.0));
			}
			if (x+1 < (int) TERRAIN_WIDTH && z-1 > 0) {
				totAng += 90.0;
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, TERRAIN_WIDTH,
						x, z, x, z-1, x+1, z-1), 45.0));
				normal = VectorAdd(normal,
					ScalarMult(normalForTriangle(vertexArray, TERRAIN_WIDTH,
						x, z, x+1, z-1, x+1, z), 45.0));
			}
			normalArray[x + z * TERRAIN_WIDTH] = ScalarMult(normal, totAng);
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

struct thing {
	vec3 pos;
	vec3 vel;
	int gravity;
	Model *model;
	GLuint texture;
};

// Global variables //

#define MAX_THINGS 1000
int num_things = 0;
struct thing things[MAX_THINGS];
Model *terrain;
GLuint program;

void drawEverything() {
	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];
		mat4 mdlMatrix = T(t->pos.x, t->pos.y, t->pos.z);
		glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, mdlMatrix.m);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, t->texture);
		DrawModel(t->model, program, "inPosition", "inNormal", "inTexCoord");
	}
}

#define GRAVITY_ACCEL 50

void updateEverything(float delta_t) {
	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];
		// Do gravity
		if (t->gravity) {
			float ground_y = terrain_height_at(t->pos.x, t->pos.z) + 10.0f;
			if (t->pos.y > ground_y) {
				// Falling
				t->vel.y -= GRAVITY_ACCEL * delta_t;
			} else {
				// Standing
				t->pos.y = ground_y;
				t->vel.y = 0.0f;
			}
		}
		// Do physics
		t->pos = VectorAdd(t->pos, ScalarMult(t->vel, delta_t));
	}
}

void createThing(const Model *model, const GLuint texture, float x, float y, float z, int gravity) {
	struct thing *t = &things[num_things++];
	t->model = model;
	t->texture = texture;
	t->pos = SetVector(x, y, z);
	t->vel = SetVector(0, 0, 0);
	t->gravity = gravity;
}

void init(void)
{
	// GL inits
	glClearColor(0.2,0.2,0.5,0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	// Enable transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, 500.0);

	// Load and compile shader
	program = loadShaders("vertex.glsl", "fragment.glsl");
	glUseProgram(program);
	printError("init shader");

	// Other setup
	glUniformMatrix4fv(glGetUniformLocation(program, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
	glUniform1i(glGetUniformLocation(program, "tex0"), 0); // Texture unit 0
	glUniform1i(glGetUniformLocation(program, "tex1"), 1); // Texture unit 1
	glUniform1i(glGetUniformLocation(program, "tex2"), 2); // Texture unit 2

	// Load textures
	GLuint maskros, concrete, dirt, grass;
	LoadTGATextureSimple("res/maskros512.tga", &maskros);
	LoadTGATextureSimple("res/conc.tga", &concrete);
	LoadTGATextureSimple("res/dirt.tga", &dirt);
	LoadTGATextureSimple("res/grass.tga", &grass);

	// Generate terrain data
	// LoadTGATextureData("fft-terrain.tga", &ttex);
	terrain = GenerateTerrain();
	printError("init terrain");

	// Load models
	Model *sphere = LoadModel("res/groundsphere.obj");
	Model *octagon = LoadModel("res/octagon.obj");
	Model *car = LoadModel("res/SPECTER_GT3_.obj");

	createThing(terrain, grass, 0, 0, 0, 0);
	createThing(car, concrete, 0, 0, 0, 1);
	createThing(sphere, maskros, 50, 50, 50, 1);
	createThing(sphere, maskros, 150, 50, 50, 1);
	createThing(sphere, maskros, 50, 50, 150, 1);
	createThing(sphere, maskros, 80, 50, 50, 1);
	createThing(sphere, maskros, 80, 50, 80, 1);
	createThing(sphere, maskros, 50, 80, 50, 1);

	// Setup light sources
	glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), lightCount, lightSourcesDirPosArr);
	glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), lightCount, lightSourcesColorArr);
	glUniform1iv(glGetUniformLocation(program, "isDirectional"), lightCount, isDirectional);
}

const vec3 up_vector = {0.0, 1.0, 0.0};
vec3 view_target = {2, 0, 2};
vec3 view_pos = {80, 5, 108};
int fogEnable = 1;

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

	glUniform1i(glGetUniformLocation(program, "fogEnable"), fogEnable);

	/*
	// Draw the ground
	modelView = IdentityMatrix();
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grass);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, dirt);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, concrete);
	DrawModel(terrain, program, "inPosition", "inNormal", "inTexCoord");
	*/

	/*
	// Draw eight copies of the ground in every direction
	const float w = TILE_WIDTH_X * ttex.width;
	const float h = TILE_WIDTH_Z * ttex.height;
	modelView = T(w, 0.0, 0.0);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	modelView = T(-w, 0.0, 0.0);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	modelView = T(0.0, 0.0, h);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	modelView = T(0.0, 0.0, -h);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	modelView = T(w, 0.0, h);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	modelView = T(-w, 0.0, -h);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	modelView = T(-w, 0.0, h);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	modelView = T(w, 0.0, -h);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	DrawModel(tm, program, "inPosition", "inNormal", "inTexCoord");
	*/

	/*
	 // Draw octagon and ball
	x = 60.0 + 30.0 * cos(t / 3.0);
	z = 60.0 + 30.0 * sin(t / 3.0);
	y = terrain_height_at(x, z);
	vec3 n = hmNormalAt(x, z);
	vec3 v = Normalize(VectorSub(SetVector(1.0, 0.0, 0.0), ScalarMult(n, n.x)));
	vec3 u = Normalize(CrossProduct(n, v));
	mat4 tiltMatrix1 = {{
		v.x, n.x, u.x, 0.0,
		v.y, n.y, u.y, 0.0,
		v.z, n.z, u.z, 0.0,
		0.0, 0.0, 0.0, 1.0,
	}};
	modelView = Mult(Mult(T(x, y, z), S(5.0, 5.0, 5.0)), tiltMatrix1);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	glBindTexture(GL_TEXTURE_2D, maskros);
	DrawModel(sphere, program, "inPosition", "inNormal", "inTexCoord");

	x = 60.0 + 50.0 * pow(cos(t / 4.0), 2);
	z = 60.0 + 50.0 * pow(sin(t / 4.0), 1);
	y = terrain_height_at(x, z);
	n = hmNormalAt(x, z);
	v = Normalize(VectorSub(SetVector(1.0, 0.0, 0.0), ScalarMult(n, n.x)));
	u = Normalize(CrossProduct(n, v));
	mat4 tiltMatrix2 = {{
		v.x, n.x, u.x, 0.0,
		v.y, n.y, u.y, 0.0,
		v.z, n.z, u.z, 0.0,
		0.0, 0.0, 0.0, 1.0,
	}};
	modelView = Mult(Mult(T(x, y, z), S(5.0, 5.0, 5.0)), tiltMatrix2);
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, modelView.m);
	glBindTexture(GL_TEXTURE_2D, maskros);
	DrawModel(octagon, program, "inPosition", "inNormal", "inTexCoord");
	*/

	drawEverything();
	printError("drawEverything");

	glutSwapBuffers();
}

float fall_speed = 0.0f;

void handleMovingControls(float delta) {
	updateEverything(delta);
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
	float ground_y = terrain_height_at(view_pos.x, view_pos.z) + 5.0f;
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

	printf("%f %f %f\n", view_pos.x, view_pos.y, view_pos.z);
}

void handleMouseLook(float delta, int dx, int dy) {
	const int MIN_PIXELS = 5;
	const float LOOK_SPEED = 300.0f;
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
	} else if (key == 'f') {
		fogEnable = !fogEnable;
	}
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
	glutInitContextVersion(3, 2);
	glutInitWindowSize(WIN_WIDTH, WIN_HEIGHT);
	glutCreateWindow("1337 Xtr3m3 R4c1ng");
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	init();
	glutTimerFunc(20, &timer, 0);

	glutHideCursor();
	glutPassiveMotionFunc(mouse);

	glutMainLoop();
	exit(0);
}
