#version 150

out vec3 normal;
out vec2 texCoord;

in vec3 inNormal;
in vec3 inPosition;
in vec2 inTexCoord;

uniform mat4 projection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

uniform float time;
uniform bool isSkybox;

void main(void)
{
	if (isSkybox) {
		gl_Position = projection * vec4(vec3(viewMatrix) * inPosition, 1.0);
	} else {
		gl_Position = projection * viewMatrix * modelMatrix * vec4(inPosition, 1.0);
	}

	normal = inNormal;

	texCoord = inTexCoord;
}
