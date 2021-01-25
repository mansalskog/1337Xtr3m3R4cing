#version 150

out float lightness;
out vec2 tex_Coord;

in vec3 in_Normal;
in vec3 in_Position;
in vec2 inTexCoord;

uniform mat4 projection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

void main(void)
{
	gl_Position = projection * viewMatrix * modelMatrix * vec4(in_Position, 1.0);

	// Gouraud shading
	mat3 normalMatrix = mat3(viewMatrix * modelMatrix);
	vec3 transfNormal = normalize(normalMatrix * in_Normal);
	vec3 lightPos = vec3(0.58, 0.58, -0.58);
	lightness = (dot(transfNormal.xyz, lightPos) + 1.0) / 2.0;

	tex_Coord = inTexCoord;
}
