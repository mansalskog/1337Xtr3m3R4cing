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

// Random number between -1 and 1
float random_unit() {
	return 2.0f * (rand() / (float) RAND_MAX) - 1.0f;
}

float norm2(float x, float y) {
	return sqrt(x * x + y * y);
}

vec3 angle_y_vec(float angle_y) {
	return SetVector(cos(angle_y), 0.0f, sin(angle_y));
}

///// Terrain (heightmap) /////

const float TERRAIN_WIDTH_FACTOR = 50.0f;
const float TERRAIN_DEPTH_FACTOR = 50.0f;
const float TERRAIN_HEIGHT_FACTOR = 15.0f;
const int TERRAIN_WIDTH = 500;
const int TERRAIN_DEPTH = 500;
const float TERRAIN_TRIANGLE_SIZE = 2.0f;

vec2 terrain_gradient(int x0, int z0) {
	// float angle = 2920.f * sin(x0 * 21942.f + z0 * 171324.f + 8912.f) * cos(x0 * 23157.f * z0 * 217832.f + 9758.f);
	srand(1337420 * x0 + 999999 * z0);
	float angle = 2.0f * M_PI * (rand() / (float) RAND_MAX);
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

	int x0 = (int) floor(x);
	int z0 = (int) floor(z);
	float dx = x - x0;
	float dz = z - z0;

	float d00 = terrain_dot_gradient(x0, z0, x, z);
	float d01 = terrain_dot_gradient(x0, z0+1, x, z);
	float d10 = terrain_dot_gradient(x0+1, z0, x, z);
	float d11 = terrain_dot_gradient(x0+1, z0+1, x, z);

	return TERRAIN_HEIGHT_FACTOR * smoothstep(smoothstep(d00, d10, dx), smoothstep(d01, d11, dx), dz);
}

/*
// Unused?
vec3 terrain_normal_at(float x, float z) {
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
*/

Model* terrain_generate_model()
{
	int vertexCount = TERRAIN_WIDTH * TERRAIN_DEPTH;
	int triangleCount = (TERRAIN_WIDTH-1) * (TERRAIN_DEPTH-1) * 2;
	int x, z;

	vec3 *vertexArray = malloc(sizeof(vec3) * vertexCount);
	vec3 *normalArray = malloc(sizeof(vec3) * vertexCount);
	vec2 *texCoordArray = malloc(sizeof(vec2) * vertexCount);
	GLuint *indexArray = malloc(sizeof(GLuint) * triangleCount * 3);

	for (x = 0; x < (int) TERRAIN_WIDTH; x++) {
		for (z = 0; z < (int) TERRAIN_DEPTH; z++)
		{
			float real_x = (x - TERRAIN_WIDTH / 2) * TERRAIN_TRIANGLE_SIZE;
			float real_z = (z - TERRAIN_DEPTH / 2) * TERRAIN_TRIANGLE_SIZE;
			vertexArray[x + z * TERRAIN_WIDTH] = SetVector(
					real_x,
					terrain_height_at(real_x, real_z),
					real_z);
			normalArray[x + z * TERRAIN_WIDTH] = SetVector(0.0, 1.0, 0.0);
			texCoordArray[x + z * TERRAIN_WIDTH].x = x; // (float)x / tex->width;
			texCoordArray[x + z * TERRAIN_WIDTH].y = z; // (float)z / tex->height;
		}
	}

	for (x = 0; x < (int) TERRAIN_WIDTH-1; x++) {
		for (z = 0; z < (int) TERRAIN_DEPTH-1; z++)
		{
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 0] = x + z * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 1] = x + (z+1) * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 2] = x+1 + z * TERRAIN_WIDTH;

			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 3] = x+1 + z * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 4] = x + (z+1) * TERRAIN_WIDTH;
			indexArray[(x + z * (TERRAIN_WIDTH-1))*6 + 5] = x+1 + (z+1) * TERRAIN_WIDTH;
		}
	}

	for (x = 0; x < (int) TERRAIN_WIDTH; x++) {
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
	}

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

// Constants

#define THING_TERRAIN 0
#define THING_ENEMY 1
#define THING_PLAYER 2

#define GRAVITY_ACCEL 50

#define CAR_ACCEL 200
#define CAR_MAX_SPEED 40
#define CAR_TURN_SPEED 0.8f
#define CAR_AIR_DRAG 0.02f

#define CAMERA_BEHIND_FAR 0
#define CAMERA_BEHIND_CLOSE 1
#define CAMERA_IN_CAR 2
#define CAMERA_MODE_LAST 2

#define MAX_THINGS 1000

struct thing {
	vec3 pos;
	vec3 vel;
	float angle_y;
	int type;
	Model *model;
	GLuint texture;
};

// Global variables //

int num_things = 0;
struct thing things[MAX_THINGS];
struct thing *player;
int camera_mode = CAMERA_BEHIND_FAR;
Model *terrain;
GLuint program;

void drawEverything() {
	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];
		mat4 mdlMatrix = Mult(
				T(t->pos.x, t->pos.y, t->pos.z),
				Ry(-t->angle_y));
		glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, mdlMatrix.m);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, t->texture);
		DrawModel(t->model, program, "inPosition", "inNormal", "inTexCoord");
	}
}

void updateEverything(float delta_t) {
	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];
		// Do gravity
		if (t->type != THING_TERRAIN) {
			float ground_y = terrain_height_at(t->pos.x, t->pos.z);
			if (t->pos.y > ground_y) {
				// Falling
				t->vel.y -= GRAVITY_ACCEL * delta_t;
			} else {
				// Standing
				t->pos.y = ground_y;
				t->vel.y = 0.0f;
			}
		}
		// Update enemies
		if (t->type == THING_ENEMY) {
			// Turn in a random direction
			t->angle_y += delta_t * CAR_TURN_SPEED * random_unit();
			t->vel = VectorAdd(t->vel, ScalarMult(angle_y_vec(t->angle_y), delta_t * CAR_ACCEL));
			// Limit speed to maxmimum
			float speed = norm2(t->vel.x, t->vel.z);
			if (speed > CAR_MAX_SPEED) {
				t->vel.x *= CAR_MAX_SPEED / speed;
				t->vel.z *= CAR_MAX_SPEED / speed;
			}
		}
		// Update the player
		if (t->type == THING_PLAYER) {
			int forward = glutKeyIsDown('w') || glutKeyIsDown(GLUT_KEY_UP);
			int left = glutKeyIsDown('a') || glutKeyIsDown(GLUT_KEY_LEFT);
			int right = glutKeyIsDown('d') || glutKeyIsDown(GLUT_KEY_RIGHT);
			// Accelerating
			if (forward) {
				player->vel = VectorAdd(player->vel, ScalarMult(angle_y_vec(player->angle_y), delta_t * CAR_ACCEL));
			}
			// Limit speed to maximum
			float speed = norm2(player->vel.x, player->vel.z);
			if (speed > CAR_MAX_SPEED) {
				player->vel.x *= CAR_MAX_SPEED / speed;
				player->vel.z *= CAR_MAX_SPEED / speed;
			}
			// Turning left and right
			if (left && speed > 0.0f) {
				player->angle_y += delta_t * CAR_TURN_SPEED;
			}
			if (right && speed > 0.0f) {
				player->angle_y -= delta_t * CAR_TURN_SPEED;
			}
			// Do friction and air drag
			player->vel = VectorAdd(player->vel, ScalarMult(player->vel, -CAR_AIR_DRAG));
		}
		// Do physics
		t->pos = VectorAdd(t->pos, ScalarMult(t->vel, delta_t));
	}
}

struct thing *createThing(const Model *model, const GLuint texture, float x, float y, float z, int type) {
	struct thing *t = &things[num_things++];
	t->model = model;
	t->texture = texture;
	t->pos = SetVector(x, y, z);
	t->vel = SetVector(0, 0, 0);
	t->type = type;
	return t;
}

void setCameraMatrix() {
	vec3 view_pos, view_target;
	if (camera_mode == CAMERA_BEHIND_FAR) {
		view_pos = VectorAdd(player->pos, VectorAdd(
				ScalarMult(angle_y_vec(player->angle_y), -50.0f),
				SetVector(0, 20, 0)));
		view_target = player->pos;
	} else if (camera_mode == CAMERA_BEHIND_CLOSE) {
		view_pos = VectorAdd(player->pos, VectorAdd(
				ScalarMult(angle_y_vec(player->angle_y), -20.0f),
				SetVector(0, 10, 0)));
		view_target = player->pos;
	} else if (camera_mode == CAMERA_IN_CAR) {
		view_pos = VectorAdd(player->pos, SetVector(0, 10, 0));
		view_target = VectorAdd(view_pos,
				ScalarMult(angle_y_vec(player->angle_y), 50.0f));
	} else {
		fprintf(stderr, "Invalid camera mode %d!\n", camera_mode);
		exit(EXIT_FAILURE);
	}
	const vec3 up_vector = {0, 1, 0};
	mat4 cameraMatrix = lookAtv(view_pos, view_target, up_vector);
	glUniformMatrix4fv(glGetUniformLocation(program, "camMatrix"), 1, GL_TRUE, cameraMatrix.m);
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

	// Generate terrain model
	terrain = terrain_generate_model();
	printError("init terrain");

	// Load models
	Model *sphere = LoadModel("res/groundsphere.obj");
	Model *octagon = LoadModel("res/octagon.obj");
	Model *car = LoadModel("res/SPECTER_GT3_.obj");

	createThing(terrain, grass, 0, 0, 0, 0);
	createThing(car, concrete, 0, 0, 0, 1);
	createThing(octagon, maskros, 50, 50, 50, THING_ENEMY);
	createThing(octagon, maskros, 60, 50, 50, THING_ENEMY);
	createThing(octagon, maskros, 50, 50, 90, THING_ENEMY);
	createThing(octagon, maskros, 80, 50, 50, THING_ENEMY);
	createThing(octagon, maskros, 80, 50, 80, THING_ENEMY);
	createThing(octagon, maskros, 50, 80, 50, THING_ENEMY);
	player = createThing(octagon, grass, 0, 0, 0, THING_PLAYER);

	// Setup light sources
	glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), lightCount, lightSourcesDirPosArr);
	glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), lightCount, lightSourcesColorArr);
	glUniform1iv(glGetUniformLocation(program, "isDirectional"), lightCount, isDirectional);
}

int fogEnable = 1;

void display(void)
{
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 total, modelView, camMatrix;
	float x, y, z;

	printError("pre display");

	glUseProgram(program);

	setCameraMatrix();

	GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	glUniform1f(glGetUniformLocation(program, "time"), t);

	glUniform1i(glGetUniformLocation(program, "fogEnable"), fogEnable);

	drawEverything();
	printError("drawEverything");

	glutSwapBuffers();
}

GLfloat prev_t = 0.0;
int paused = 0;

void timer(int i)
{
	glutTimerFunc(20, &timer, i);
    GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	updateEverything(t - prev_t);
	prev_t = t;
	glutPostRedisplay();
}

void mouse(int x, int y)
{
}

void keyboard(unsigned char key, int x, int y) {
	if (key == GLUT_KEY_ESC) {
		// Handle pause menu
	} else if (key == 'f') {
		fogEnable = !fogEnable;
	} else if (key == 'c') {
		camera_mode = (camera_mode + 1) % (CAMERA_MODE_LAST + 1);
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

	// glutHideCursor();
	glutPassiveMotionFunc(mouse);

	glutMainLoop();
	exit(0);
}
