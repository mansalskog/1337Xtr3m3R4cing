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
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 800;

const float TILE_WIDTH_X = 3.0;
const float TILE_WIDTH_Z = 3.0;
const float TILE_HEIGHT_Y = 0.5;

void restart_game(void);

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

float clamp(float x, float min, float max) {
	return fmaxf(min, fminf(x, max));
}

float normalize_angle(float angle) {
	return asin(sin(angle));
}

///// Terrain (heightmap) /////

const float TERRAIN_WIDTH_FACTOR = 200.0f;
const float TERRAIN_DEPTH_FACTOR = 200.0f;
const float TERRAIN_HEIGHT_FACTOR = 25.0f;
const int TERRAIN_WIDTH = 1000;
const int TERRAIN_DEPTH = 1000;
const float TERRAIN_TRIANGLE_SIZE = 2.0f;

vec2 terrain_gradient(int x0, int z0) {
	unsigned next_seed = rand();
	srand(1337420 * x0 + 999999 * z0);
	float angle = 2.0f * M_PI * (rand() / (float) RAND_MAX);
	srand(next_seed);
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

vec2 SetVector2(float x, float y) {
	vec2 v = {x, y};
	return v;
}

Model *generate_particle_model() {
	const int vertexCount = 6;
	const int triangleCount = 2;

	vec3 *vertexArray = malloc(sizeof(vec3) * vertexCount);
	vec3 *normalArray = malloc(sizeof(vec3) * vertexCount);
	vec2 *texCoordArray = malloc(sizeof(vec2) * vertexCount);
	GLuint *indexArray = malloc(sizeof(GLuint) * triangleCount * 3);

	vertexArray[0] = SetVector(-1, -1, 0);
	vertexArray[1] = SetVector( 1, -1, 0);
	vertexArray[2] = SetVector(-1,  1, 0);
	vertexArray[3] = SetVector( 1, -1, 0);
	vertexArray[4] = SetVector( 1,  1, 0);
	vertexArray[5] = SetVector(-1,  1, 0);

	normalArray[0] = SetVector(0, 0, -1);
	normalArray[1] = SetVector(0, 0, -1);
	normalArray[2] = SetVector(0, 0, -1);
	normalArray[3] = SetVector(0, 0, -1);
	normalArray[4] = SetVector(0, 0, -1);
	normalArray[5] = SetVector(0, 0, -1);

	texCoordArray[0] = SetVector2(0, 1);
	texCoordArray[1] = SetVector2(1, 1);
	texCoordArray[2] = SetVector2(0, 0);
	texCoordArray[3] = SetVector2(1, 1);
	texCoordArray[4] = SetVector2(1, 0);
	texCoordArray[5] = SetVector2(0, 0);

	indexArray[0] = 0;
	indexArray[1] = 1;
	indexArray[2] = 2;
	indexArray[3] = 3;
	indexArray[4] = 4;
	indexArray[5] = 5;

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
#define THING_OBSTACLE 3

#define GRAVITY_ACCEL 30

#define CAR_ACCEL 100
#define CAR_ROAD_ACCEL 200
#define CAR_BRAKE_ACCEL 50
#define CAR_MAX_SPEED 100
#define CAR_ROAD_MAX_SPEED 200

// Minimum speed required to turn
#define CAR_MIN_TURN_SPEED 3
#define CAR_TURN_SPEED 0.8f
#define ENEMY_TURN_SPEED 1.2f

#define CAR_AIR_DRAG 0.10f
#define CAR_AIR_DRAG_EXTRA 0.50f

#define CAMERA_BEHIND_FAR 0
#define CAMERA_BEHIND_CLOSE 1
#define CAMERA_IN_FRONT 2
#define CAMERA_IN_CAR 3
#define CAMERA_ABOVE_CAR 4
#define CAMERA_ABOVE_MAP 5
#define CAMERA_MODE_LAST 5

#define FAR_PLANE_DIST 1500.0f
#define FOG_COLOR 0.86, 0.86, 0.89, 0.0

// Max particles must be at least NUM_ENEMIES * particle lifetime * particles per car = 5 * 4 * 50
#define MAX_PARTICLES 10000
#define MAX_THINGS 1000
#define MAX_THING_TEXTURES 4
#define NUM_TERRAIN_OBJS 40
#define NUM_ENEMIES 3
#define NUM_WAYPOINTS 20
// Must be a divisor of NUM_WAYPOINTS
#define PLAYER_WAYPOINT_SKIP 2

#define ROAD_WIDTH 25.0f
#define WAYPOINT_DETECT_RADIUS 40.0f

#define MAX_LIGHTS 30
#define LIGHT_NONE 0
#define LIGHT_POSITION 1
#define LIGHT_DIRECTION 2
#define LIGHTS_PER_THING 4

#define NUM_OF_LAPS 3

struct thing {
	vec3 pos;
	vec3 lastPos;
	vec3 vel;
	float angle_y;
	int type;
	float air_height;
	vec3 last_normal;
	int nextWaypoint;
	Model *model;
	GLuint textures[MAX_THING_TEXTURES];
    mat4 baseMdlMatrix;
	float radius;
	int lightIndex[LIGHTS_PER_THING];
	int laps;
	float notMovedTime;
	float reverseTime;
	float reverseTurnAngle;
};

struct particle {
	vec3 pos;
	vec3 vel;
	vec3 color;
	float size;
	float lifetime;
};

// Global variables //

int num_things = 0;
struct thing things[MAX_THINGS];
struct thing *player;
struct thing *terrain;
int idx_particle = 0;
struct particle particles[MAX_PARTICLES] = {0};
Model *particle_model;
int camera_mode = CAMERA_BEHIND_FAR;
GLuint program;
vec3 waypoints[NUM_WAYPOINTS];

int paused = 1;
int cheats = 0;
int fogEnable = 1;

int lastLightIndex = 0;
GLfloat lightSourcesDirPosArr[3*MAX_LIGHTS] = {0};
GLfloat lightSourcesColorArr[3*MAX_LIGHTS] = {0};
int lightSourcesTypeArr[MAX_LIGHTS] = {0};

mat4 projectionMatrix;

vec3 view_pos;
vec3 view_target;

int oldNumCarsBefore = 0;
float newPositionAlertStart = 0.0f;
int oldPlayerLaps = 0;
float newLapAlertStart = 0.0f;
int racePosition = -1;
float raceEndedAt = 0.0f;

// Loaded assets

// World textures
GLuint concrete, dirt, grass, fencetex,
	   barrel1, barrel2, car1, car2, car3, car4, fence1, fence2,
	   grass1, grass2, road1, road2, stone1, stone2, tire1;

// World models
Model *sphere, *octagon, *car, *tree, *rock, *oildrum, *tires, *fence;

// User interface textures
GLuint currentPlaceTex[NUM_ENEMIES + 1];
GLuint newPlaceTex[NUM_ENEMIES + 1];
GLuint pausedTex, finishedTex;
GLuint newLapTex[NUM_OF_LAPS];

int seenByCamera(vec3 pos) {
	// Check if angle is less than 90 degrees, which approximately is what we can see.
	vec3 camToPos = VectorSub(pos, view_pos);
	float angle = DotProduct(Normalize(camToPos), Normalize(VectorSub(view_target, view_pos)));
	return Norm(camToPos) < FAR_PLANE_DIST
		&& fabs(acos(angle)) < M_PI / 4.0;
}

void setLight(int index, vec3 posDir, vec3 color, int type) {
	if (index >= MAX_LIGHTS) {
		fprintf(stderr, "Too many lights: %d!\n", index);
		return;
	}
	lightSourcesDirPosArr[3*index] = posDir.x;
	lightSourcesDirPosArr[3*index+1] = posDir.y;
	lightSourcesDirPosArr[3*index+2] = posDir.z;
	lightSourcesColorArr[3*index] = color.x;
	lightSourcesColorArr[3*index+1] = color.y;
	lightSourcesColorArr[3*index+2] = color.z;
	lightSourcesTypeArr[index] = type;
}

void drawEverything() {
	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];

		// Skip things "out of view", except terrain and player
		if (t->type != THING_TERRAIN && t->type != THING_PLAYER) {
			if (!seenByCamera(t->pos)) continue;
		}

		mat4 mdlMatrix;
		if (t->type == THING_TERRAIN || t->type == THING_OBSTACLE) {
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
		glUniform1i(glGetUniformLocation(program, "hl_wp"), player->nextWaypoint);
		DrawModel(t->model, program, "inPosition", "inNormal", "inTexCoord");

		char buf[200];
		sprintf(buf, "draw thing %d", i);
		printError(buf);
	}
}

void drawParticles() {
	glUniform1i(glGetUniformLocation(program, "isParticle"), 1);
	for (int i = 0; i < MAX_PARTICLES; i++) {
		struct particle *p = &particles[i];

		// Skip particles "out of view"
		if (!seenByCamera(p->pos)) continue;

		if (p->lifetime > 0.0f) {
			mat4 mdlMatrix = T(p->pos.x, p->pos.y, p->pos.z);
			glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, mdlMatrix.m);
			glUniform1f(glGetUniformLocation(program, "particleLifetime"), p->lifetime);
			glUniform1f(glGetUniformLocation(program, "particleSize"), p->size);
			glUniform3f(glGetUniformLocation(program, "particleColor"), p->color.x, p->color.y, p->color.z);
			DrawModel(particle_model, program, "inPosition", "inNormal", "inTexCoord");
		}
	}
	glUniform1i(glGetUniformLocation(program, "isParticle"), 0);
}

int numberOfCarsBeforePlayer() {
	int before = 0;
	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];
		if (t->type == THING_ENEMY) {
			if (t->laps > player->laps) {
				before++;
			} else if (t->laps == player->laps) {
				if ((t->nextWaypoint-1)%NUM_WAYPOINTS > (player->nextWaypoint-1)%NUM_WAYPOINTS) {
					before++;
				} else if ((t->nextWaypoint-1)%NUM_WAYPOINTS == (player->nextWaypoint-1)%NUM_WAYPOINTS) {
						// || (t->nextWaypoint + 1) % NUM_WAYPOINTS == player->nextWaypoint) {
					// This code assumes that PLAYER_WAYPOINT_SKIP == 2
					vec3 wp = waypoints[player->nextWaypoint];
					if (Norm(VectorSub(wp, t->pos)) < Norm(VectorSub(wp, player->pos))) {
						before++;
					}
				}
			}
		}
	}
	return before;
}

void drawTextureToScreen(GLuint texture, mat4 transfMatrix) {
	glUniformMatrix4fv(glGetUniformLocation(program, "mdlMatrix"), 1, GL_TRUE, transfMatrix.m);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texture);
	DrawModel(particle_model, program, "inPosition", "inNormal", "inTexCoord");
}

void drawUserInterface() {
	glUniform1i(glGetUniformLocation(program, "isUserInterface"), 1);

	int numCarsBefore = numberOfCarsBeforePlayer();
	if (numCarsBefore < 0 || NUM_ENEMIES < numCarsBefore) {
		fprintf(stderr, "Invalid number of cars before %d!\n", numCarsBefore);
	}
	drawTextureToScreen(currentPlaceTex[numCarsBefore], Mult(S(1.0f, 0.5f, 1.0f), T(0.0f, -2.5f, 0.0f)));

	GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	if (numCarsBefore != oldNumCarsBefore) {
		// pretty ugly code, update show a message if the position changes
		oldNumCarsBefore = numCarsBefore;
		newPositionAlertStart = t;
	}
	if (player->laps != oldPlayerLaps) {
		oldPlayerLaps = player->laps;
		newLapAlertStart = t;
	}

	if (paused) {
		// Show pause "menu"
		drawTextureToScreen(pausedTex, Mult(S(2.0f, 1.0f, 1.0f), T(0.75f, 0.0f, 0.0f)));
	} else if (racePosition != -1) {
		// Show race end text
		drawTextureToScreen(finishedTex, Mult(S(1.0f, 0.5f, 1.0f), T(0.65f, 0.5f, 0.0f)));
	} else if (t - newLapAlertStart < 0.5f && player->laps - 1 < NUM_OF_LAPS) {
		// Show new lap alert for 0.5 seconds
		drawTextureToScreen(newLapTex[player->laps - 1], Mult(S(1.0f, 0.5f, 1.0f), T(0.5f, 0.5f, 0.0f)));
	} else if (t - newPositionAlertStart < 0.5f) {
		// Show new position alert for 0.5 seconds
		drawTextureToScreen(newPlaceTex[numCarsBefore], Mult(S(1.0f, 0.5f, 1.0f), T(0.5f, 0.5f, 0.0f)));
	}


	glUniform1i(glGetUniformLocation(program, "isUserInterface"), 0);
}

void addParticle(vec3 pos, vec3 vel, float lifetime, vec3 color, float size) {
	struct particle *p = &particles[idx_particle];
	idx_particle = (idx_particle + 1) % MAX_PARTICLES;
	p->pos = pos;
	p->vel = vel;
	p->lifetime = lifetime;
	p->color = color;
	p->size = size;
}

void updateParticles(float delta) {
	for (int i = 0; i < MAX_PARTICLES; i++) {
		struct particle *p = &particles[i];
		if (p->lifetime > 0.0f) {
			p->lifetime -= delta;
			p->pos = VectorAdd(p->pos, ScalarMult(p->vel, delta));
		}
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
	// Restart the race
	float time = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	if (player->laps == NUM_OF_LAPS + 1 && racePosition == -1) {
		raceEndedAt = time;
		racePosition = numberOfCarsBeforePlayer();
	}
	if (racePosition != -1 && time - raceEndedAt > 3.0f) {
		printf("RESTARTNG GAME!!!!!\n");
		restart_game();
		return;
	}

	for (int i = 0; i < num_things; i++) {
		struct thing *t = &things[i];
		// Do gravity
		if (t->type != THING_TERRAIN && t->type != THING_OBSTACLE) {
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
		// Update enemies driving if on ground
		if (t->type == THING_ENEMY) { // && t->air_height > 0.0f) {
			// Turn towards next waypoint
			vec3 v_to_wp = VectorSub(waypoints[t->nextWaypoint], t->pos);
			float angle_to_wp = atan2(v_to_wp.z, v_to_wp.x);
			float angle_diff = normalize_angle(angle_to_wp - t->angle_y);
			if (fabs(angle_diff) > 0.05f && t->reverseTime <= 0.0f) {
				t->angle_y += clamp(angle_diff, -delta_t * ENEMY_TURN_SPEED, delta_t * ENEMY_TURN_SPEED);
			}

			// Drive faster on road
			float max_accel = CAR_ACCEL;
			if (isOnRoad(t->pos)) {
				max_accel = CAR_ROAD_ACCEL;
			}
			// """ PD - control system """
			const float P = 10.0f;
			const float D = 0.0f;
			float accel = max_accel / 5.0f; // P * Norm(v_to_wp) + D * Norm(t->vel);
			if (angle_diff < 0.10f) {
				accel = max_accel;
			}

			// Check if we're stationary
			if (Norm(VectorSub(t->pos, t->lastPos)) < delta_t * CAR_MAX_SPEED / 5.0f) {
				t->notMovedTime += delta_t;
			} else {
				t->notMovedTime = 0.0f;
			}
			// If stationary for more than 3 seconds, start reversing
			if (t->notMovedTime > 3.0f) {
				t->reverseTime = 1.5f;
				switch (rand() % 3) {
					case 0:
						t->reverseTurnAngle = -ENEMY_TURN_SPEED;
						break;
					case 1:
						t->reverseTurnAngle = 0.0f;
						break;
					case 2:
						t->reverseTurnAngle = ENEMY_TURN_SPEED;
						break;
				}
			}
			// Reverse if we're reversing (hack to make enemies not stuck)
			if (t->reverseTime > 0.0f) {
				accel = -CAR_BRAKE_ACCEL;
				t->angle_y += t->reverseTurnAngle * delta_t;
				t->reverseTime -= delta_t;
			}

			// Limit speed to maximum
			accel = clamp(accel, -CAR_BRAKE_ACCEL, max_accel);
			t->vel = VectorAdd(t->vel,
							   ScalarMult(angle_y_vec(t->angle_y),
										  delta_t * accel));
			// Select new waypoint if close enough
			if (Norm(v_to_wp) < WAYPOINT_DETECT_RADIUS / 2.0f) {
				if (t->nextWaypoint == 0) {
					// We have gone around one time
					t->laps++;
				}
				t->nextWaypoint = (t->nextWaypoint + 1) % NUM_WAYPOINTS;
			}
		}
		// Update the player driving if on ground
		if (t->type == THING_PLAYER) { // && t->air_height > 0.0f) {
			int forward = glutKeyIsDown('w') || glutKeyIsDown(GLUT_KEY_UP);
			int left = glutKeyIsDown('a') || glutKeyIsDown(GLUT_KEY_LEFT);
			int right = glutKeyIsDown('d') || glutKeyIsDown(GLUT_KEY_RIGHT);
            int brake = glutKeyIsDown('s') || glutKeyIsDown(GLUT_KEY_DOWN);
			// Drive faster on road
			float accel = CAR_ACCEL;
			if (isOnRoad(t->pos)) {
				accel = CAR_ROAD_ACCEL;
			}
			if (cheats) {
				accel = 3.0f * CAR_ROAD_ACCEL;
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
			int goingForward = DotProduct(t->vel, angle_y_vec(t->angle_y)) > 0.0f;
			// Turning left and right
			if (((goingForward && left) || (!goingForward && right)) && speed > CAR_MIN_TURN_SPEED) {
				t->angle_y -= delta_t * CAR_TURN_SPEED;
			}
			if (((goingForward && right) || (!goingForward && left)) && speed > CAR_MIN_TURN_SPEED) {
				t->angle_y += delta_t * CAR_TURN_SPEED;
			}
			// Record new waypoint if close enough
			vec3 v_to_wp = VectorSub(waypoints[t->nextWaypoint], t->pos);
			if (Norm(v_to_wp) < WAYPOINT_DETECT_RADIUS) {
				// Create particles when waypoint is reached
				for (int i = 0; i < 500; i++) {
					const float RADIUS = 40.0f;
					float angle = random_range(0, 2.0f * M_PI);
					vec3 pos = VectorAdd(
							waypoints[player->nextWaypoint],
							ScalarMult(angle_y_vec(angle), RADIUS));
					addParticle(pos,
							SetVector(0.0f, random_range(8.0f, 12.0f), 0.0f),
							random_range(1.8f, 2.2f),
							SetVector(0.83f, 0.67f, 0.22f),
							1.0f);
				}
				if (t->nextWaypoint == 0) {
					// Gone around one time, assuming that PLAYER_WAYPOINT_SKIP is a multiple of NUM_WAYPOINTS
					t->laps++;
				}
				// Player only needs to hit some waypoints
				t->nextWaypoint = (t->nextWaypoint + PLAYER_WAYPOINT_SKIP) % NUM_WAYPOINTS;
			}
		}
		if (t->type == THING_ENEMY || t->type == THING_PLAYER) {
			// Limit speed to maximum
			float max_speed = CAR_MAX_SPEED;
			if (isOnRoad(t->pos)) {
				max_speed = CAR_ROAD_MAX_SPEED;
			}
			if (cheats && t->type == THING_PLAYER) {
				max_speed = 3.0f * CAR_ROAD_MAX_SPEED;
			}
			const float speed = norm2(t->vel.x, t->vel.z);
			float air_drag = CAR_AIR_DRAG;
			if (speed > max_speed) {
				air_drag = CAR_AIR_DRAG_EXTRA;
			}
			// Do friction and air drag
			t->vel = VectorAdd(t->vel, ScalarMult(t->vel, -air_drag));

			// Do physics
			for (int j = 0; j < num_things; j++) {
				if (i == j) continue;
				struct thing *s = &things[j];
				vec3 offset = VectorSub(s->pos, t->pos);
				if (Norm(offset) < t->radius + s->radius) {
					// Thing t hits thing s, so project t->vel on orthogonal complement of offset
					// But we don't want to change the velocity in the y-axis
					float y_vel = t->vel.y;
					t->vel = VectorSub(t->vel, ScalarMult(offset, fmax(0.0f, DotProduct(t->vel, offset)) / DotProduct(offset, offset)));
					t->vel.y = y_vel;
				}
			}
			// Save position before update
			t->lastPos = t->pos;
			t->pos = VectorAdd(t->pos, ScalarMult(t->vel, delta_t));

			// Create smoke from tires
			if (speed > CAR_MIN_TURN_SPEED) {
				for (int i = 0; i < 50; i++) {
					addParticle(
							VectorAdd(t->pos, ScalarMult(angle_y_vec(t->angle_y + random_range(-1.0f, 1.0f)), -random_range(1.5f, 3.0f))),
							VectorAdd(ScalarMult(angle_y_vec(t->angle_y), 3.0f), SetVector(0.0f, 5.0f, 0.0f)),
							random_range(0.8f, 1.2f),
							SetVector(0.11f, 0.11f, 0.11f),
							random_range(0.35f, 0.40f));
				}
			}

			// Move headlight to in front of and behind the car
			const vec3 FRONT_LIGHT_COLOR = SetVector(0.78f, 0.91f, 1.0f);
			const vec3 BACK_LIGHT_COLOR = SetVector(0.9f, 0.2f, 0.2f);
			if (t->type == THING_PLAYER) {
				setLight(t->lightIndex[0],
						VectorAdd(VectorAdd(t->pos, SetVector(0, 0.5f, 0)), ScalarMult(angle_y_vec(t->angle_y + 0.6f), 4.0f)),
						FRONT_LIGHT_COLOR,
						LIGHT_POSITION);
				setLight(t->lightIndex[1],
						VectorAdd(VectorAdd(t->pos, SetVector(0, 0.5f, 0)), ScalarMult(angle_y_vec(t->angle_y - 0.5f), 4.0f)),
						FRONT_LIGHT_COLOR,
						LIGHT_POSITION);
				setLight(t->lightIndex[2],
						VectorAdd(VectorAdd(t->pos, SetVector(0, 0.5f, 0)), ScalarMult(angle_y_vec(t->angle_y + 0.6f), -3.0f)),
						BACK_LIGHT_COLOR,
						LIGHT_POSITION);
				setLight(t->lightIndex[3],
						VectorAdd(VectorAdd(t->pos, SetVector(0, 0.5f, 0)), ScalarMult(angle_y_vec(t->angle_y - 0.5f), -3.0f)),
						BACK_LIGHT_COLOR,
						LIGHT_POSITION);
			} else if (t->type == THING_ENEMY && seenByCamera(t->pos)) {
				// Use only two lights for enemies
				setLight(t->lightIndex[0],
						VectorAdd(VectorAdd(t->pos, SetVector(0, 0.5f, 0)), ScalarMult(angle_y_vec(t->angle_y), 4.0f)),
						FRONT_LIGHT_COLOR,
						LIGHT_POSITION);
				setLight(t->lightIndex[1],
						VectorAdd(VectorAdd(t->pos, SetVector(0, 0.5f, 0)), ScalarMult(angle_y_vec(t->angle_y), -3.0f)),
						BACK_LIGHT_COLOR,
						LIGHT_POSITION);
			} else {
				// Turn off all lights for far away cars
				for (int i = 0; i < LIGHTS_PER_THING; i++) {
					setLight(t->lightIndex[i],
							SetVector(0.0f, 0.0f, 0.0f),
							SetVector(0.0f, 0.0f, 0.0f),
							LIGHT_NONE);
				}
			}
		}
	}
}

struct thing *createThing(float x, float y, float z,
						  int type,
                          Model *model, mat4 baseMdlMatrix,
						  GLuint tex0, GLuint tex1, GLuint tex2, GLuint tex3,
						  float radius, float angle_y) {
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
    t->radius = radius;
	t->nextWaypoint = 0;
	t->laps = 0;
	t->notMovedTime = 0.0f;
	t->reverseTime = 0.0f;
	t->angle_y = angle_y;
	if (type == THING_ENEMY || type == THING_PLAYER) {
		for (int i = 0; i < LIGHTS_PER_THING; i++) {
			t->lightIndex[i] = lastLightIndex++;
		}
	}
	return t;
}

void setCameraMatrix() {
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
		case CAMERA_IN_FRONT:
			view_pos = VectorAdd(player->pos, VectorAdd(
						ScalarMult(angle_y_vec(player->angle_y), 25.0f),
						SetVector(0, 10, 0)));
			view_target = player->pos;
			break;
		case CAMERA_IN_CAR:
			view_pos = VectorAdd(player->pos, SetVector(0, 10, 0));
			view_target = VectorAdd(view_pos,
					ScalarMult(angle_y_vec(player->angle_y), 50.0f));
			break;
		case CAMERA_ABOVE_CAR:
			view_pos = VectorAdd(player->pos, SetVector(0, 50, 0));
			view_target = player->pos;
			up_vector = angle_y_vec(player->angle_y);
			break;
		case CAMERA_ABOVE_MAP:
			view_pos = SetVector(0, 1000, 0);
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

// Setup OpenGL stuff and load models and textures
void init(void)
{
	// GL inits
	glClearColor(FOG_COLOR);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printError("GL inits");

	// Enable transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	projectionMatrix = frustum(-0.1, 0.1, -0.1, 0.1, 0.2, FAR_PLANE_DIST);

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

	// Load user interface textures
	LoadTGATextureSimple("res/1st.tga", &currentPlaceTex[0]);
	LoadTGATextureSimple("res/2nd.tga", &currentPlaceTex[1]);
	LoadTGATextureSimple("res/3rd.tga", &currentPlaceTex[2]);
	LoadTGATextureSimple("res/4th.tga", &currentPlaceTex[3]);
	LoadTGATextureSimple("res/pausedtex.tga", &pausedTex);
	LoadTGATextureSimple("res/1stplace.tga", &newPlaceTex[0]);
	LoadTGATextureSimple("res/2ndplace.tga", &newPlaceTex[1]);
	LoadTGATextureSimple("res/3rdplace.tga", &newPlaceTex[2]);
	LoadTGATextureSimple("res/4thplace.tga", &newPlaceTex[3]);

	// Load textures
	LoadTGATextureSimple("res/conc.tga", &concrete);
	LoadTGATextureSimple("res/dirt.tga", &dirt);
	LoadTGATextureSimple("res/grass.tga", &grass);
    LoadTGATextureSimple("res/fence2-tex.tga", &fencetex);
    LoadTGATextureSimple("res/barrel1-tex.tga", &barrel1);
	LoadTGATextureSimple("res/barrel2-tex.tga", &barrel2);
	LoadTGATextureSimple("res/car1-tex.tga", &car1);
	LoadTGATextureSimple("res/car2-tex.tga", &car2);
    LoadTGATextureSimple("res/car3-tex.tga", &car3);
    LoadTGATextureSimple("res/car4-tex.tga", &car4);
	LoadTGATextureSimple("res/fence1-tex.tga", &fence1);
	LoadTGATextureSimple("res/fence2-tex.tga", &fence2);
	LoadTGATextureSimple("res/grass1-tex.tga", &grass1);
    LoadTGATextureSimple("res/grass2-tex.tga", &grass2);
    LoadTGATextureSimple("res/road1-tex.tga", &road1);
	LoadTGATextureSimple("res/road2-tex.tga", &road2);
	LoadTGATextureSimple("res/stone1-tex.tga", &stone1);
	LoadTGATextureSimple("res/stone2-tex.tga", &stone2);
    LoadTGATextureSimple("res/tire1-tex.tga", &tire1);
    LoadTGATextureSimple("res/finished.tga", &finishedTex);
    LoadTGATextureSimple("res/lap1.tga", &newLapTex[0]);
    LoadTGATextureSimple("res/lap2.tga", &newLapTex[1]);
    LoadTGATextureSimple("res/lap3.tga", &newLapTex[2]);

	// Generate particle model
	particle_model = generate_particle_model();
	printError("init particle model");

	// Load models
	sphere = LoadModel("res/groundsphere.obj");
	octagon = LoadModel("res/octagon.obj");
	car = LoadModel("res/artega_gt.obj");
	tree = LoadModel("res/cgaxis_models_115_37_obj.obj");
	rock = LoadModel("res/Rock_1.obj");
	oildrum = LoadModel("res/barrel.obj.obj");
	tires = LoadModel("res/wheel.obj");
    fence = LoadModel("res/old_fence.obj");
}

void restart_game(void) {
	// Reset all global variables
	racePosition = -1;

	num_things = 0;
	idx_particle = 0;
	// camera_mode = CAMERA_BEHIND_FAR;

	paused = 1;
	// cheats = 0;
	// fogEnable = 1;

	lastLightIndex = 0;

	oldNumCarsBefore = 0;
	newPositionAlertStart = 0.0f;

	oldPlayerLaps = 0;
	newLapAlertStart = 0.0f;

	// Reset all global lists
	memset(things, 0, sizeof *things * MAX_THINGS);
	memset(particles, 0, sizeof *particles * MAX_PARTICLES);
	// memset(waypoints, 0, sizeof *waypoints * NUM_WAYPOINTS);
	memset(lightSourcesDirPosArr, 0, sizeof *lightSourcesDirPosArr * 3*MAX_LIGHTS);
	memset(lightSourcesColorArr, 0, sizeof *lightSourcesColorArr * 3*MAX_LIGHTS);
	memset(lightSourcesTypeArr, 0, sizeof *lightSourcesTypeArr * MAX_LIGHTS);

	// Set random seed
	srand(time(0));

	// Create global lights
	setLight(lastLightIndex++, SetVector(-1.0f, -1.0f, -1.0f), SetVector(0.94f, 0.56f, 0.22f), LIGHT_DIRECTION);
	// setLight(lastLightIndex++, SetVector(10.0f, 10.0f, 10.0f), SetVector(1.0f, 1.0f, 1.0f), LIGHT_POSITION);

	// Generate terrain model
	Model *terrainMdl = terrain_generate_model();
	// Create terrain (ground)
	terrain = createThing(0, 0, 0,
			THING_TERRAIN,
			terrainMdl, IdentityMatrix(),
			grass2, road2, grass, grass,
			0.0f, 0.0f);
	printError("init terrain");

	// Create waypoints
	float r = TERRAIN_WIDTH / 2.0f;
	float min_wp_r = r;
	float max_wp_r = r;
	for (int i = 0; i < NUM_WAYPOINTS; i++) {
		const float VAR = 0.15f;
		float dr;
		if (r < TERRAIN_WIDTH / 2.0f) {
			dr = random_range(0.0f, VAR * r);
		} else if (r > TERRAIN_WIDTH / 1.0f) {
			dr = random_range(-VAR * r, 0.0f);
		} else {
			dr = random_range(-VAR * r, VAR * r);
		}
		r += dr;
		// Slightly less than 2 * M_PI
		float angle = (6.0f * i) / (float) NUM_WAYPOINTS;
		float x = r * cos(angle);
		float z = r * sin(angle);
		waypoints[i] = SetVector(x, terrain_height_at(x, z), z);
		min_wp_r = fmin(r, min_wp_r);
		max_wp_r = fmax(r, max_wp_r);
	}

	// Create fence around road
	for (int i = 0; i < NUM_WAYPOINTS; i++) {
		vec3 lastPoint;
		vec3 beforeLastPoint;
		if (i == 0) {
			lastPoint = waypoints[NUM_WAYPOINTS-1];
			beforeLastPoint = waypoints[NUM_WAYPOINTS-2];
		} else if (i == 1) {
			lastPoint = waypoints[0];
			beforeLastPoint = waypoints[NUM_WAYPOINTS-1];
		} else {
			lastPoint = waypoints[i-1];
			beforeLastPoint = waypoints[i-2];
		}
		vec3 v = VectorSub(waypoints[i], lastPoint);
		float l = Norm(v);
		v = Normalize(v);
		vec3 u = Normalize(CrossProduct(v, terrain_normal_at(v.x, v.z)));
		const float FENCE_WIDTH = 7.0f;

		float turn_angle = acos(DotProduct(
					Normalize(VectorSub(waypoints[i], lastPoint)),
					Normalize(VectorSub(lastPoint, beforeLastPoint))));
		// Inner fence
		float margin = sin(turn_angle) * 5.0f * FENCE_WIDTH;
		float t = margin;
		const float RADIUS = 4.0f;
		while (t + 2.0f * margin < l) {
			vec3 pos = VectorAdd(lastPoint, VectorAdd(ScalarMult(v, t), ScalarMult(u, 2.0f * ROAD_WIDTH)));
			if (!isOnRoad(pos)) {
				mat4 modelMat = Mult(Ry(M_PI / 2.0f + atan2(v.x, v.z)), S(0.1f, 0.1f, 0.1f));
				createThing(pos.x, terrain_height_at(pos.x, pos.z), pos.z,
							THING_OBSTACLE,
							fence, modelMat,
							fencetex, concrete, concrete, concrete,
							RADIUS, 0.0f);
			}
			t += FENCE_WIDTH;
		}
		margin = sin(turn_angle) * 1.5f * FENCE_WIDTH;
		t = margin;
		// Outer fence
		while (t + margin < l) {
			vec3 pos = VectorAdd(lastPoint, VectorAdd(ScalarMult(v, t), ScalarMult(u, -2.0f * ROAD_WIDTH)));
			if (!isOnRoad(pos)) {
				mat4 modelMat = Mult(Ry(M_PI / 2.0f + atan2(v.x, v.z)), S(0.1f, 0.1f, 0.1f));
				createThing(pos.x, terrain_height_at(pos.x, pos.z), pos.z,
							THING_OBSTACLE,
							fence, modelMat,
							fencetex, concrete, concrete, concrete,
							RADIUS, 0.0f);
			}
			t += FENCE_WIDTH;
		}
	}

	// Upload waypoints to GPU
	// TODO: Maybe not completely ok to cast points?
	glUniform3fv(glGetUniformLocation(program, "waypoints"), NUM_WAYPOINTS, (GLfloat *) waypoints);
	printError("upload waypoints");

	// Create static terrain (trees and rocks)
	for (int i = 0; i < NUM_TERRAIN_OBJS; i++) {
		// Pick a random waypoint
		vec3 wp = waypoints[rand() % NUM_WAYPOINTS];
		const float RADIUS = 70.0f;
		float angle = random_range(0, 2.0f * M_PI);
		float x = wp.x + RADIUS * cos(angle);
		float z = wp.z + RADIUS * sin(angle);
		Model *model;
		mat4 modelMatr;
		float radius;
        GLuint tex;
		vec3 pos = SetVector(x, terrain_height_at(x, z), z);
		int max_rng = 2;
		if (!isOnRoad(pos)) {
			max_rng = 4; // Only make trees and rocks if not on the road
		}
		switch (rand() % max_rng) {
			case 0:
				model = oildrum;
				radius = 8.0f;
				modelMatr = S(0.5f, 0.5f, 0.5f);
                tex = barrel1;
				break;
			case 1:
				model = tires;
				radius = 2.0f;
				modelMatr = Mult(T(0.0f, 0.2f, 0.0f), Mult(S(0.1f, 0.1f, 0.1f), Rx(M_PI / 2.0f)));
                tex = tire1;
				break;
			case 2:
				model = tree;
				radius = 4.0f;
				modelMatr = S(0.5f, 0.5f, 0.5f);
                tex = grass1;
				break;
			case 3:
				model = rock;
				radius = 10.0f;
				modelMatr = S(0.3f, 0.3f, 0.3f);
                tex = stone1;
				break;
		}
		createThing(pos.x, pos.y, pos.z,
                    THING_OBSTACLE,
					model, modelMatr,
					tex, tex, tex, tex,
					radius, 0.0f);
	}

	// Create enemies
	for (int i = 0; i < NUM_ENEMIES; i++) {
		const float DEV = 10.0f;
		float x = waypoints[0].x + random_range(-DEV, DEV);
		float z = waypoints[0].z + random_range(-DEV, DEV);
		GLuint tex;
		switch (i) {
			case 1:
				tex = car1;
				break;
			case 2:
				tex = car2;
				break;
			case 3:
				tex = car3;
				break;
		}
		float angle_to_wp = atan2(waypoints[1].z - z, waypoints[1].x - x);
		createThing(x, waypoints[0].y, z,
				THING_ENEMY,
				car, Mult(S(2, 2, 2), Ry(M_PI / 2.0f)),
				tex, tex, tex, tex,
				5.0f, angle_to_wp);
	}

	float player_angle_to_wp = atan2(waypoints[1].z - waypoints[0].z, waypoints[1].x - waypoints[0].x);
	player = createThing(waypoints[0].x, waypoints[0].y, waypoints[0].z,
			THING_PLAYER,
			car, Mult(S(2, 2, 2), Ry(M_PI / 2.0f)),
			car4, car4, car4, car4,
			5.0f, player_angle_to_wp);
}

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

	// The map camera cannot use fog
	glUniform1i(glGetUniformLocation(program, "fogEnable"), fogEnable && camera_mode != CAMERA_ABOVE_MAP);

	// Update light sources
	glUniform3fv(glGetUniformLocation(program, "lightSourcesDirPosArr"), MAX_LIGHTS, lightSourcesDirPosArr);
	glUniform3fv(glGetUniformLocation(program, "lightSourcesColorArr"), MAX_LIGHTS, lightSourcesColorArr);
	glUniform1iv(glGetUniformLocation(program, "lightSourcesTypeArr"), MAX_LIGHTS, lightSourcesTypeArr);

	drawEverything();
	printError("drawEverything");

	drawParticles();
	printError("drawParticles");

	drawUserInterface();
	printError("drawUserInterface");

	glutSwapBuffers();
}

GLfloat prev_t = 0.0;

void timer(int i)
{
	glutTimerFunc(20, &timer, i);
    GLfloat t = (GLfloat) glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	if (!paused) {
		updateEverything(t - prev_t);
		updateParticles(t - prev_t);
	}
	prev_t = t;
	glutPostRedisplay();
}

void mouse(int x, int y)
{
}

void keyboard(unsigned char key, int x, int y) {
	if (key == GLUT_KEY_ESC) {
		// Handle pause menu
		paused = !paused;
	} else if (key == 'f') {
		fogEnable = !fogEnable;
	} else if (key == 'c') {
		camera_mode = (camera_mode + 1) % (CAMERA_MODE_LAST + 1);
	} else if (key == 'b') {
		cheats = !cheats;
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
	restart_game();
	glutTimerFunc(20, &timer, 0);

	// glutHideCursor();
	glutPassiveMotionFunc(mouse);

	glutMainLoop();
	exit(0);
}
