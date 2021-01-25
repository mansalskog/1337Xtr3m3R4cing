#version 150

in vec3 vert_Color;
in vec2 tex_Coord;

out vec4 out_Color;

uniform float time;
uniform sampler2D texUnit;

void main(void)
{
	out_Color = vec4(sin(tex_Coord.x * 50.0), sin(tex_Coord.y * 50.0), abs(sin(time)), 1.0);
	vec2 new_tex_Coord = vec2(abs(sin(tex_Coord.x * 3.0)), abs(sin(tex_Coord.y * 3.0)));
	out_Color = texture(texUnit, new_tex_Coord);
}
