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
uniform bool isUserInterface;

uniform float particleLifetime;
uniform float particleSize;

void main(void)
{
	// mat3 normalMatrix1 = mat3(camMatrix * mdlMatrix);

	vec4 worldPosition = mdlMatrix * vec4(inPosition, 1.0);
	vec4 viewPosition = camMatrix * worldPosition;
	gl_Position = projMatrix * viewPosition;

	if (isParticle) {
		// Only translate, always facing the camera (plane)
		mat4 totalMat = projMatrix * camMatrix * mdlMatrix;
		float s = clamp(particleLifetime, 0.0, 1.0) * particleSize;
		totalMat[0] = vec4(  s, 0.0, 0.0, 0.0);
		totalMat[1] = vec4(0.0,   s, 0.0, 0.0);
		totalMat[2] = vec4(0.0, 0.0,   s, 0.0);
		gl_Position = totalMat * vec4(inPosition, 1.0);
	} else if (isUserInterface) {
		// No transformation except model matrix, draw in screen space directly
		gl_Position = mdlMatrix * vec4(inPosition, 1.0);
	}

	viewPos = vec3(viewPosition);
	worldPos = vec3(worldPosition);
	normal = inNormal;
	texCoord = inTexCoord;
}
