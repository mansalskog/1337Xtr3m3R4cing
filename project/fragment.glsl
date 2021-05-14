#version 150

in vec2 texCoord;
in vec3 normal;
in vec3 viewPos;
in vec3 worldPos;

out vec4 outColor;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;

uniform mat4 camMatrix;
uniform mat4 mdlMatrix;

uniform float time;

// Lighting
#define MAX_LIGHTS 30
#define LIGHT_NONE 0
#define LIGHT_POSITION 1
#define LIGHT_DIRECTION 2
uniform vec3 lightSourcesDirPosArr[MAX_LIGHTS];
uniform vec3 lightSourcesColorArr[MAX_LIGHTS];
uniform int lightSourcesTypeArr[MAX_LIGHTS];

// Road
#define NUM_WAYPOINTS 20
#define ROAD_WIDTH 25.0
#define WAYPOINT_DETECT_RADIUS 40.0
uniform vec3 waypoints[NUM_WAYPOINTS];
uniform int hl_wp;

#define THING_TERRAIN 0
#define THING_ENEMY 1
#define THING_PLAYER 2
uniform int thingType;

uniform bool isParticle;
uniform bool isUserInterface;

uniform float particleLifetime;
uniform vec3 particleColor;

#define FAR_PLANE_DIST 500.0
#define FOG_FADE_DIST 100.0
#define FOG_COLOR 0.86, 0.86, 0.89, 0.0
uniform bool fogEnable;

void main(void)
{
	// Light model constants
	float reflectivity = 1.0; // Should depend on model?
	float specularity = 1.0;
	float specularExponent = 50.0;

	// Phong lighting model
	mat3 normalMatrix1 = mat3(camMatrix * mdlMatrix);
	vec3 transfNormal = normalize(normalMatrix1 * normal);

	vec3 ambientLight = reflectivity * vec3(0.3);
	vec3 diffuseLight = vec3(0.0);
	vec3 specularLight = vec3(0.0);
	for (int i = 0; i < MAX_LIGHTS; i++) {
		if (lightSourcesTypeArr[i] == LIGHT_NONE) continue;

		// Direction or position of the light in view coordinates
		// Light positions have no model matrix
		vec3 lightDirPos = (camMatrix * vec4(lightSourcesDirPosArr[i], 1.0)).xyz;

		// Direction towards the light
		vec3 dirToLight;
		float attenuation;
		if (lightSourcesTypeArr[i] == LIGHT_DIRECTION) {
		   dirToLight = normalize(lightDirPos);
		   attenuation = 1.0;
		} else if (lightSourcesTypeArr[i] == LIGHT_POSITION) {
		   dirToLight = normalize(lightDirPos - viewPos);
		   float d = length(lightDirPos - viewPos);
		   float a = 1.0;
		   float b = 0.1;
		   float c = 0.01;
		   attenuation = 1.0 / (a + b*d + c*d*d);
		}
		diffuseLight += max(vec3(0.0), reflectivity * lightSourcesColorArr[i] * dot(dirToLight, transfNormal)) * attenuation;

		// s mirrored through transfNormal
		vec3 r = normalize(2.0 * transfNormal * dot(dirToLight, transfNormal) - dirToLight);
		// Vector towards the camera, assuming the camera is at the orgin in view coords
		vec3 v = normalize(vec3(0.0) - viewPos);
		specularLight += max(vec3(0.0), specularity * lightSourcesColorArr[i] * pow(dot(r, v), specularExponent)) * attenuation;
	}
	vec4 totalLight = vec4(ambientLight + diffuseLight + specularLight, 1.0);

	// Regular texture
	vec4 color = texture(tex0, texCoord);

	// Multitexturing for terrain (i.e. roads)
	if (thingType == THING_TERRAIN) {
		// Highlight waypoints
		vec3 offset = worldPos - waypoints[hl_wp];
		if (length(offset) < WAYPOINT_DETECT_RADIUS && length(offset) > WAYPOINT_DETECT_RADIUS * 0.9) {
			color = vec4(0.83, 0.67, 0.22, 1.0);
		} else {
			// Generate noise to get smoother edges
			float noise = 2.0 * fract(sin(dot(worldPos.xz, vec2(12.9898,78.233))) * 43758.5453);

			// Draw the road
			for (int i = 0; i < NUM_WAYPOINTS; i++) {
				// Line segment from waypoints[i-1] to waypoints[i]
				vec3 v;
				if (i == 0) {
					v = waypoints[i] - waypoints[NUM_WAYPOINTS-1];
				} else {
					v = waypoints[i] - waypoints[i-1];
				}
				vec3 u = worldPos - waypoints[i];
				vec3 proj = dot(u, v) / dot(v, v) * v;
				float d = dot(proj, v);
				bool onRoad = false;
				if (-dot(v, v) < d && d < 0.0 && length(u - proj) < ROAD_WIDTH + noise) {
					onRoad = true;
				} else if (length(u) < ROAD_WIDTH + noise) {
					onRoad = true;
				}
				if (onRoad) {
					// Draw goal line
					if (i == 0 && length(proj) < 5.0) {
						if (mod(length(u - proj), 4.0) < 2.0) { 
							color = vec4(0.1, 0.1, 0.1, 1.0);
						} else {
							color = vec4(0.9, 0.9, 0.9, 1.0);
						}
					} else {
						color = texture(tex1, texCoord);
					}
					break;
				}
			}
		}
	}

	outColor = color * totalLight;

	float fogStartDist = FAR_PLANE_DIST - FOG_FADE_DIST;
	if (fogEnable && length(viewPos) > fogStartDist) {
		// Fade out over FOG_FADE_DIST units
		float t = (length(viewPos) - fogStartDist) / FOG_FADE_DIST;
		t = smoothstep(0.0, 1.0, t);
		outColor = mix(outColor, vec4(FOG_COLOR), t);
	}

	if (isParticle) {
		// Skip texture, use plain color
		float alpha = 0.8 * clamp(particleLifetime, 0.0, 1.0);
		outColor = vec4(particleColor, alpha) * totalLight;
	} else if (isUserInterface) {
		// Skip all lighting, just use the texture
		outColor = texture(tex0, texCoord);
	}
}
