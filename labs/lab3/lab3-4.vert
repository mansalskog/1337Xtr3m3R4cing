#version 150

out vec3 normal;
out vec3 viewPosition;
out vec2 texCoord;

in vec3 inNormal;
in vec3 inPosition;
in vec2 inTexCoord;

uniform mat4 projection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

uniform float time;
uniform bool isSkybox;

uniform float specularExponent;
uniform vec3 lightSourcesDirPosArr[4];
uniform vec3 lightSourcesColorArr[4];
uniform bool isDirectional[4];

void main(void)
{
	vec4 viewPos;
	if (isSkybox) {
        // remove translation from view matrix, and remove entire model matrix
		viewPos = mat4(mat3(viewMatrix)) * vec4(inPosition, 1.0);
	} else {
		viewPos = viewMatrix * modelMatrix * vec4(inPosition, 1.0);
	}

	gl_Position = projection * viewPos;

	viewPosition = viewPos.xyz;
	normal = inNormal;
	texCoord = inTexCoord;
}
