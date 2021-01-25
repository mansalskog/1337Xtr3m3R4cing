#version 150

in vec3 normal;
in vec2 texCoord;

out vec4 outColor;

uniform float time;
uniform sampler2D texUnit;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

void main(void)
{
	// Phong shading
	mat3 normalMatrix = mat3(viewMatrix * modelMatrix);
	vec3 transfNormal = normalize(normalMatrix * normal);
	vec3 lightPos = vec3(0.58, 0.58, -0.58);
	float lightness = (dot(transfNormal.xyz, lightPos) + 1.0) / 2.0;

	outColor = lightness * texture(texUnit, texCoord);
}
