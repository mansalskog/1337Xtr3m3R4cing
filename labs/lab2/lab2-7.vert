#version 150

out vec3 normal;
out vec2 texCoord;

in vec3 inNormal;
in vec3 inPosition;
in vec2 inTexCoord;

uniform mat4 projection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

void main(void)
{
	gl_Position = projection * viewMatrix * modelMatrix * vec4(inPosition, 1.0);

	normal= inNormal;

	texCoord = inTexCoord;
}
