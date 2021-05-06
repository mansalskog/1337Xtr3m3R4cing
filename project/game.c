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
#include <time.h>

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

float random_range(float min, float max) {
	return (max - min) * random_unit() + min;
}

float norm2(float x, float y) {
	return sqrt(x * x + y * y);
}

vec3 angle_y_vec(float angle_y) {
	return SetVector(cos(angle_y), 0.0f, sin(angle_y));
}

///// Terrain (heightmap) /////

const float TERRAIN_WIDTH_FACTOR = 200.0f;
const float TERRAIN_DEPTH_FACTOR = 200.0f;
const float TERRAIN_HEIGHT_FACTOR = 15.0f;
const int TERRAIN_WIDTH = 1000;
const int TERRAIN_DEPTH = 1000;
const float TERRAIN_TRIANGLE_SIZE = 2.0f; 

// Decided at startup, used to seed random
unsigned global_seed;

vec2 terrain_gradient(int x0, int z0) {
	srand(global_seed + 1337420 * x0 + 999999 * z0);
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

vec3 terrain_normal_at(float x, float z) {
	const float delta = 0.001;
	float h = terrain_height_at(x, z);
	vec3 npp = Normalize(CrossProduct(
			SetVector(delta, terrain_height_at(x + delta, z) - h, 0),
			SetVector(0, terrain_height_at(x, z + delta) - h, delta)));
	vec3 nmp = Normalize(CrossProduct(
			SetVector(0, terrain_height_at(x, z + delta) - h, delta),
			SetVector(-delta, terrain_height_at(x - delta, z) - h, 0)));
	vec3 nmm = Normalize(CrossProduct(
			SetVector(-delta, terrain_height_at(x - delta, z) - h, 0),
			SetVector(0, terrain_height_at(x, z - delta) - h, -delta)));
	vec3 npm = Normalize(CrossProduct(
			SetVector(0, terrain_height_at(x, z - delta) - h, -delta),
			SetVector(delta, terrain_height_at(x + delta, z) - h, 0)));

	return ScalarMult(Normalize(VectorAdd(VectorAdd(npp, nmp), VectorAdd(nmm, nmp))), -1.0f);
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

#define GRAVITY_ACCEL 30

#define CAR_ACCEL 100
#define CAR_ROAD_ACCEL 200
#define CAR_BRAKE_ACCEL 50
#define CAR_MAX_SPEED 100
#define CAR_ROAD_MAX_SPEED 200

// Minimum speed required to turn
#define CAR_MIN_TURN_SPEED 3
#define CAR_TURN_SPEED 0.8f

#define CAR_AIR_DRAG 0.10f
#define CAR_AIR_DRAG_EXTRA 0.50f

#define CAMERA_BEHIND_FAR 0
#define CAMERA_BEHIND_CLOSE 1
#define CAMERA_IN_CAR 2
#define CAMERA_ABOVE_CAR 3
#define CAMERA_ABOVE_MAP 4
#define CAMERA_MODE_LAST 4

#define MAX_THINGS 1000
#define MAX_THING_TEXTURES 4
#define NUM_TERRAIN_OBJS 40
#define NUM_ENEMIES 50
#define NUM_WAYPOINTS 50

#define ROAD_WIDTH 25.0f

struct thing {
	vec3 pos;
	vec3 vel;
	float angle_y;
	int type;
	float air_height;
	vec3 last_normal;
	Model *model;
	GLuint textures[MAX_THING_TEXTURES];
    mat4 baseMdlMatrix;
};

// Global variables //

int num_things = 0;
struct thing things[MAX_THINGS];
struct thing *player;
struct thing *terrain;
int camera_mode = CAMERA_ABOVE_CAR;
GLuint program;
vec3 waypoints[NUM_WAYPOINTS];

void drawEverything() {
	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];
		mat4 mdlMatrix;
		if (t->type == THING_TERRAIN) {
			mdlMatrix = Mult(
                Mult(T(t->pos.x, t->pos.y, t->pos.z),
                     Ry(-t->angle_y)),
                t->baseMdlMatrix);
		} else if (t->type != THING_TERRAIN) {
			vec3 n = terrain_normal_at(t->pos.x, t->pos.z);
			// If we're in the air, use the last normal vector from when on the ground
			if (t->air_height > 0.0f) {
				n = Normalize(VectorAdd(
							ScalarMult(t->last_normal, t->air_height),
							ScalarMult(n, 1.0f / t->air_height)));
			} else {
				t->last_normal = n;
			}
			vec3 v = Normalize(VectorSub(SetVector(1.0, 0.0, 0.0), ScalarMult(n, n.x)));
			vec3 u = Normalize(CrossProduct(v, n));
			mat4 tiltMatrix = {{
				v.x, n.x, u.x, 0.0,
				v.y, n.y, u.y, 0.0,
				v.z, n.z, u.z, 0.0,
				0.0, 0.0, 0.0, 1.0,
			}};
			mdlMatrix = Mult(
					Mult(T(t->pos.x, t->pos.y, t->pos.z), tiltMatrix),
					Mult(Ry(-t->angle_y), t->baseMdlMatrix));
		}
		glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, mdlMatrix.m);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, t->textures[0]);
        glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, t->textures[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, t->textures[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, t->textures[3]);
		glUniform1i(glGetUniformLocation(program, "thingType"), t->type);
		DrawModel(t->model, program, "inPosition", "inNormal", "inTexCoord");
	}
}

int isOnRoad(vec3 pos) {
	for (int i = 0; i < NUM_WAYPOINTS; i++) {
		// Line segment from waypoints[i-1] to waypoints[i]
		vec3 v;
		if (i == 0) {
			v = VectorSub(waypoints[i], waypoints[NUM_WAYPOINTS-1]);
		} else {
			v = VectorSub(waypoints[i], waypoints[i-1]);
		}
		vec3 u = VectorSub(pos, waypoints[i]);
		vec3 proj = ScalarMult(v, DotProduct(u, v) / DotProduct(v, v));
		float d = DotProduct(proj, v);
		if (-DotProduct(v, v) < d && d < 0.0 && Norm(VectorSub(u, proj)) < ROAD_WIDTH) {
			return 1;
		}
		if (Norm(u) < ROAD_WIDTH) {
			return 1;
		}
	}
	return 0;
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
				t->air_height = t->pos.y - ground_y;
			} else {
				// Standing
				t->pos.y = ground_y;
				t->vel.y = 0.0f;
				t->air_height = 0.0f;
			}
		}
		// Update enemies
		if (t->type == THING_ENEMY) {
			// Turn in a random direction
			// t->angle_y += delta_t * CAR_TURN_SPEED * random_unit();

			// Drive faster on road
			float accel = CAR_ACCEL;
			if (isOnRoad(t->pos)) {
				accel = CAR_ROAD_ACCEL;
			}
			srand(time(0));
			if (rand() > RAND_MAX / 2) {
				t->vel = VectorAdd(t->vel,
								   ScalarMult(angle_y_vec(t->angle_y),
											  delta_t * accel));
			}
		}
		// Update the player
		if (t->type == THING_PLAYER) {
			int forward = glutKeyIsDown('w') || glutKeyIsDown(GLUT_KEY_UP);
			int left = glutKeyIsDown('a') || glutKeyIsDown(GLUT_KEY_LEFT);
			int right = glutKeyIsDown('d') || glutKeyIsDown(GLUT_KEY_RIGHT);
            int brake = glutKeyIsDown('s') || glutKeyIsDown(GLUT_KEY_DOWN);

			// Drive faster on road
			float accel = CAR_ACCEL;
			if (isOnRoad(t->pos)) {
				accel = CAR_ROAD_ACCEL;
			}

			// Accelerating
			if (forward) {
				t->vel = VectorAdd(t->vel,
								   ScalarMult(angle_y_vec(t->angle_y),
											  delta_t * accel));
			}
			// Brake / reverse
            if (brake) {
                t->vel = VectorSub(t->vel,
								   ScalarMult(angle_y_vec(t->angle_y),
                                              delta_t * CAR_BRAKE_ACCEL));
            }
			const float speed = norm2(t->vel.x, t->vel.z);
			// Turning left and right
			if (left && speed > CAR_MIN_TURN_SPEED) {
				t->angle_y -= delta_t * CAR_TURN_SPEED;
			}
			if (right && speed > CAR_MIN_TURN_SPEED) {
				t->angle_y += delta_t * CAR_TURN_SPEED;
			}
		}
		if (t->type == THING_ENEMY || t->type == THING_PLAYER) {
			// Limit speed to maximum
			float max_speed = CAR_MAX_SPEED;
			if (isOnRoad(t->pos)) {
				max_speed = CAR_ROAD_MAX_SPEED;
			}
			const float speed = norm2(t->vel.x, t->vel.z);
			float air_drag = CAR_AIR_DRAG;
			if (speed > max_speed) {
				air_drag = CAR_AIR_DRAG_EXTRA;
			}
			// Do friction and air drag
			t->vel = VectorAdd(t->vel, ScalarMult(t->vel, -air_drag));
			// Do physics
			t->pos = VectorAdd(t->pos, ScalarMult(t->vel, delta_t));
		}
	}
}

struct thing *createThing(float x, float y, float z,
						  int type,
                          Model *model, mat4 baseMdlMatrix,
						  GLuint tex0, GLuint tex1, GLuint tex2, GLuint tex3) {
	struct thing *t = &things[num_things++];
	t->model = model;
	t->textures[0] = tex0;
	t->textures[1] = tex1;
	t->textures[2] = tex2;
	t->textures[3] = tex3;
	t->pos = SetVector(x, y, z);
	t->vel = SetVector(0, 0, 0);
	t->type = type;
    t->baseMdlMatrix = baseMdlMatrix;
	return t;
}

void setCameraMatrix() {
	vec3 view_pos, view_target;
	vec3 up_vector = {0, 1, 0};
	switch (camera_mode) {
		case CAMERA_BEHIND_FAR:
			view_pos = VectorAdd(player->pos, VectorAdd(
						ScalarMult(angle_y_vec(player->angle_y), -30.0f),
						SetVector(0, 10, 0)));
			view_target = player->pos;
			break;
		case CAMERA_BEHIND_CLOSE:
			view_pos = VectorAdd(player->pos, VectorAdd(
						ScalarMult(angle_y_vec(player->angle_y), -15.0f),
						SetVector(0, 5, 0)));
			view_target = player->pos;
			break;
		case CAMERA_IN_CAR:
			view_pos = VectorAdd(player->pos, SetVector(0, 10, 0));
			view_target = VectorAdd(view_pos,
					ScalarMult(angle_y_vec(player->angle_y), 50.0f));
			break;
		case CAMERA_ABOVE_CAR:
			view_pos = VectorAdd(player->pos, SetVector(0, 200, 0));
			view_target = player->pos;
			up_vector = angle_y_vec(player->angle_y);
			break;
		case CAMERA_ABOVE_MAP:
			view_pos = SetVector(0, 1400, 0);
			view_target = SetVector(0, 0, 0);
			up_vector = SetVector(1, 0, 0);
			break;
		default:
			fprintf(stderr, "Invalid camera mode %d!\n", camera_mode);
			exit(EXIT_FAILURE);
	}
	// Make sure camera is not underneath the ground
	float min_height = terrain_height_at(view_pos.x, view_pos.z) + 5.0f;
	if (view_pos.y < min_height) {
		view_pos.y = min_height;
	}
	mat4 cameraMatrix = lookAtv(view_pos, view_target, up_vector);
	glUniformMatrix4fv(glGetUniformLocation(program, "camMatrix"), 1, GL_TRUE, cameraMatrix.m);
}

void init(void)
{
	// Get a random seed
	srand(time(0));
	global_seed = rand();

	// GL inits
	glClearColor(0.2,0.2,0.5,0);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	// Enable transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, 1500.0);

	// Load and compile shader
	program = loadShaders("vertex.glsl", "fragment.glsl");
	glUseProgram(program);
	printError("init shader");

	// Other setup
	glUniformMatrix4fv(glGetUniformLocation(program, "projMatrix"), 1, GL_TRUE, projectionMatrix.m);
	glUniform1i(glGetUniformLocation(program, "tex0"), 0); // Texture unit 0
	glUniform1i(glGetUniformLocation(program, "tex1"), 1); // Texture unit 1
	glUniform1i(glGetUniformLocation(program, "tex2"), 2); // Texture unit 2
	glUniform1i(glGetUniformLocation(program, "tex3"), 3); // Texture unit 3

	// Load textures
	GLuint maskros, concrete, dirt, grass;
	LoadTGATextureSimple("res/maskros512.tga", &maskros);
	LoadTGATextureSimple("res/conc.tga", &concrete);
	LoadTGATextureSimple("res/dirt.tga", &dirt);
	LoadTGATextureSimple("res/grass.tga", &grass);

	// Generate terrain model
	Model *terrainMdl = terrain_generate_model();
	printError("init terrain");

	// Load models
	Model *sphere = LoadModel("res/groundsphere.obj");
	Model *octagon = LoadModel("res/octagon.obj");
	Model *car = LoadModel("res/artega_gt.obj");
	Model *tree = LoadModel("res/cgaxis_models_115_37_obj.obj");
	Model *rock = LoadModel("res/Rock_1.obj");
	Model *oildrum = LoadModel("res/barrel.obj.obj");
	Model *tires = LoadModel("res/wheel.obj");

	// Create terrain (ground)
	terrain = createThing(0, 0, 0, THING_TERRAIN, terrainMdl, IdentityMatrix(), grass, dirt, concrete, maskros);

	// Create waypoints
	float r = TERRAIN_WIDTH / 2.0f;
	for (int i = 0; i < NUM_WAYPOINTS; i++) {
		const float VAR = 0.15f;
		float dr;
		if (r < TERRAIN_WIDTH / 3.0f) {
			dr = random_range(0.0f, VAR * r);
		} else if (r > TERRAIN_WIDTH / 1.5f) {
			dr = random_range(-VAR * r, 0.0f);
		} else {
			dr = random_range(-VAR * r, VAR * r);
		}
		r += dr;
		float angle = (M_PI * 2.0f * i) / (float) NUM_WAYPOINTS;
		float x = r * cos(angle);
		float z = r * sin(angle);
		waypoints[i] = SetVector(x, terrain_height_at(x, z), z);
	}

	// Upload waypoints to GPU
	// TODO: Maybe not completely ok to cast points?
	glUniform3fv(glGetUniformLocation(program, "waypoints"), NUM_WAYPOINTS, (GLfloat *) waypoints);
	printError("upload waypoints");


	// Create static terrain (trees and rocks)
	for (int i = 0; i < NUM_TERRAIN_OBJS; i++) {
		// Pick a random waypoint
		vec3 wp = waypoints[rand() % NUM_WAYPOINTS];
		const float DEV = 10.0f;
		float x = wp.x + random_range(-DEV, DEV);
		float z = wp.z + random_range(-DEV, DEV);
		Model *model;
		switch (rand() % 4) {
			case 0:
				model = tree;
				break;
			case 1:
				model = rock;
				break;
			case 2:
				model = oildrum;
				break;
			case 3:
				model = tires;
				break;
		}
		createThing(x, terrain_height_at(x, z), z,
                    THING_TERRAIN,
					model, S(0.5f, 0.5f, 0.5f),
					concrete, dirt, grass, maskros);
	}

	// Create enemies
	for (int i = 0; i < NUM_ENEMIES; i++) {
		const float DEV = 10.0f;
		float x = waypoints[0].x + random_range(-DEV, DEV);
		float z = waypoints[0].z + random_range(-DEV, DEV);
		createThing(x, waypoints[0].y + 50.0f, z,
				THING_ENEMY,
				car, Mult(S(2, 2, 2), Ry(M_PI / 2.0f)),
				concrete, dirt, grass, maskros);
	}

	player = createThing(waypoints[0].x, waypoints[0].y + 50.0f, waypoints[0].z,
			THING_PLAYER,
			car, Mult(S(2, 2, 2), Ry(M_PI / 2.0f)),
			dirt, concrete, grass, maskros);


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
