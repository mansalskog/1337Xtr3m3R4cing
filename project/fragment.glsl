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
uniform bool fogEnable;

// Lighting
#define NUM_LIGHTS 2
uniform vec3 lightSourcesDirPosArr[NUM_LIGHTS];
uniform vec3 lightSourcesColorArr[NUM_LIGHTS];
uniform bool isDirectional[NUM_LIGHTS];

// Road
#define NUM_WAYPOINTS 50
#define ROAD_WIDTH 25.0
#define WAYPOINT_DETECT_RADIUS 40.0
uniform vec3 waypoints[NUM_WAYPOINTS];
uniform int hl_wp;

#define THING_TERRAIN 0
#define THING_ENEMY 1
#define THING_PLAYER 2
uniform int thingType;

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
	for (int i = 0; i < NUM_LIGHTS; i++) {
		// Direction or position of the light in view coordinates
		vec3 lightDirPos = (camMatrix * mdlMatrix * vec4(lightSourcesDirPosArr[i], 1.0)).xyz;

		// Direction towards the light
		vec3 s;
		if (isDirectional[i]) {
		   s = normalize(lightDirPos);
		} else {
		  s = normalize(lightDirPos - viewPos);
		}

		diffuseLight += reflectivity * lightSourcesColorArr[i] * dot(s, transfNormal);

		// s mirrored through transfNormal
		vec3 r = normalize(2.0 * transfNormal * dot(s, transfNormal) - s);
		// Vector towards the camera, assuming the camera is at the orgin in view coords
		vec3 v = normalize(vec3(0.0) - viewPos);
		specularLight += specularity * lightSourcesColorArr[i] * pow(dot(r, v), specularExponent);
	}
	vec4 totalLight = vec4(ambientLight + diffuseLight + specularLight, 1.0);

	// Regular texture
	vec4 color = texture(tex0, texCoord);

	// Multitexturing for terrain (i.e. roads)
	if (thingType == THING_TERRAIN) {
		// Highlight waypoints
		vec3 offset = worldPos - waypoints[hl_wp];
		if (length(offset) < WAYPOINT_DETECT_RADIUS && length(offset) > WAYPOINT_DETECT_RADIUS * 0.9) {
			color = texture(tex2, texCoord);
		} else {
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
				if (-dot(v, v) < d && d < 0.0 && length(u - proj) < ROAD_WIDTH) {
					color = texture(tex1, texCoord);
					break;
				}
				if (length(u) < ROAD_WIDTH) {
					color = texture(tex1, texCoord);
					break;
				}
			}
		}
	}
	outColor = color * totalLight;

	const float fogDistance = 100.0;
	if (fogEnable && length(viewPos) > fogDistance) {
		// Fade out over 50 units
		float t = (length(viewPos) - fogDistance) / 50.0;
		t = smoothstep(0.0, 1.0, t);
		// outColor = mix(outColor, vec4(1.0, 1.0, 1.0, 0.0), t);
	}
}
