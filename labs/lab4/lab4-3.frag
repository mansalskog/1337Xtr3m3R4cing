#version 150

in vec2 texCoord;
in vec3 normal;
in vec3 viewPos;

out vec4 outColor;

uniform sampler2D tex;
uniform mat4 mdlMatrix;

// Lighting
uniform vec3 lightSourcesDirPosArr[2];
uniform vec3 lightSourcesColorArr[2];
uniform bool isDirectional[2];

void main(void)
{
	// Light model constants
	float reflectivity = 1.0; // Should depend on model?
	float specularity = 1.0;
	float specularExponent = 50.0;

	// Phong lighting model
	mat3 normalMatrix1 = mat3(mdlMatrix);
	vec3 transfNormal = normalize(normalMatrix1 * normal);

	vec3 ambientLight = reflectivity * vec3(0.3);
	vec3 diffuseLight = vec3(0.0);
	vec3 specularLight = vec3(0.0);
	for (int i = 0; i < 4; i++) {
		// Direction or position of the light in view coordinates
		vec3 lightDirPos = (mdlMatrix * vec4(lightSourcesDirPosArr[i], 1.0)).xyz;

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

	outColor = texture(tex, texCoord) * totalLight;
}
