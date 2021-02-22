#version 150

in vec3 normal;
in vec3 viewPosition;
in vec2 texCoord;

out vec4 outColor;

uniform float time;
uniform sampler2D texUnit;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

uniform bool useTexture;
uniform bool isSkybox;

uniform float specularExponent;
uniform vec3 lightSourcesDirPosArr[4];
uniform vec3 lightSourcesColorArr[4];
uniform bool isDirectional[4];

void main(void)
{
	// Light model constants
	float reflectivity = 1.0; // Should depend on model?
	float specularity = 1.0;

	// Phong lighting model
	mat3 normalMatrix = mat3(viewMatrix * modelMatrix);
	vec3 transfNormal = normalize(normalMatrix * normal);

	vec3 ambientLight = reflectivity * vec3(0.3);
	vec3 diffuseLight = vec3(0.0);
	vec3 specularLight = vec3(0.0);
	for (int i = 0; i < 4; i++) {
		// Direction or position of the light in view coordinates
		vec3 lightDirPos = (viewMatrix * vec4(lightSourcesDirPosArr[i], 1.0)).xyz;

		// Direction towards the light
		vec3 s;
		if (isDirectional[i]) {
		   s = normalize(lightDirPos);
		} else {
		  s = normalize(lightDirPos - viewPosition);
		}

		diffuseLight += reflectivity * lightSourcesColorArr[i] * dot(s, transfNormal);

		// s mirrored through transfNormal
		vec3 r = normalize(2.0 * transfNormal * dot(s, transfNormal) - s);
		// Vector towards the camera, assuming the camera is at the orgin in view coords
		vec3 v = normalize(vec3(0.0) - viewPosition);
		specularLight += specularity * lightSourcesColorArr[i] * pow(dot(r, v), specularExponent);
	}
	vec4 totalLight = vec4(ambientLight + diffuseLight + specularLight, 1.0);
	vec4 transparency = vec4(1.0, 1.0, 1.0, 0.7);

	if (isSkybox) {
	   // No lighting for skybox
	   outColor = texture(texUnit, texCoord);
	} else if (useTexture) {
		outColor = texture(texUnit, texCoord) * transparency * totalLight;
	} else {
		outColor = transparency * totalLight;
	}
}
