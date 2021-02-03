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
                // remove translation from view matrix, and remove entire model matrix
		gl_Position = projection * mat4(mat3(viewMatrix)) * vec4(inPosition, 1.0);
	} else {
		gl_Position = projection * viewMatrix * modelMatrix * vec4(inPosition, 1.0);
	}

	normal = inNormal;

	texCoord = inTexCoord;
}
