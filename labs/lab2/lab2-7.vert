#version 150

out vec3 normal;
out vec2 texCoord;

in vec3 inNormal;
in vec3 inPosition;
in vec2 inTexCoord;

uniform mat4 projection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

uniform bool useTexture;
uniform float time;

void main(void)
{
	if (useTexture) {
		gl_Position = projection * viewMatrix * modelMatrix * vec4(inPosition, 1.0);
	} else {
		vec4 delta = 0.3 * vec4(0.0, 0.8 * sin(pow(1.5 * sin(time), 2.0) * inPosition.z), 0.0, 0.0);
		gl_Position = projection * viewMatrix * (delta + modelMatrix * vec4(inPosition, 1.0));
	}

	normal = inNormal;

	texCoord = inTexCoord;
}
