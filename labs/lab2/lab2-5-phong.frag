#version 150

in vec3 normalVec;
in vec2 tex_Coord;

out vec4 out_Color;

uniform float time;
uniform sampler2D texUnit;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

void main(void)
{
	vec2 new_tex_Coord = vec2(abs(sin(tex_Coord.x * 3.0)), abs(sin(tex_Coord.y * 3.0)));

	// Phong shading
	mat3 normalMatrix = mat3(viewMatrix * modelMatrix);
	vec3 transfNormal = normalize(normalMatrix * normalVec);
	vec3 lightPos = vec3(0.58, 0.58, -0.58);
	float lightness = (dot(transfNormal.xyz, lightPos) + 1.0) / 2.0;

	out_Color = lightness * vec4(1.0); // texture(texUnit, new_tex_Coord);
}
