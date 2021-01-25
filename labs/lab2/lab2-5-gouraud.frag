#version 150

in float lightness;
in vec2 tex_Coord;

out vec4 out_Color;

uniform float time;
uniform sampler2D texUnit;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

void main(void)
{
	vec2 new_tex_Coord = vec2(abs(sin(tex_Coord.x * 3.0)), abs(sin(tex_Coord.y * 3.0)));

	out_Color = lightness * vec4(1.0); // texture(texUnit, new_tex_Coord);
}
