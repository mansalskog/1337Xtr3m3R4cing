#version 150

in vec3 inPosition;
in vec3 inNormal;
in vec2 inTexCoord;

out vec2 texCoord;
out vec3 normal;
out vec3 viewPos;
out vec3 worldPos;

uniform mat4 projMatrix;
uniform mat4 camMatrix;
uniform mat4 mdlMatrix;

uniform bool isParticle;

void main(void)
{
	mat3 normalMatrix1 = mat3(camMatrix * mdlMatrix);
	texCoord = inTexCoord;
	vec4 worldPosition = mdlMatrix * vec4(inPosition, 1.0);
	if (isParticle) {
		// Only translate, always facing the camera...
		// worldPosition = worldPosition + vec4(mdlMatrix[3][0], mdlMatrix[3][1], mdlMatrix[3][2], 0.0);
	}
	vec4 viewPosition = camMatrix * worldPosition;
	gl_Position = projMatrix * viewPosition;
	viewPos = vec3(viewPosition);
	worldPos = vec3(worldPosition);
	normal = inNormal;
}
