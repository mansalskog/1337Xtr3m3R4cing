#version 150

in vec3 vert_Color;
in vec2 tex_Coord;

out vec4 out_Color;

uniform float time;

void main(void)
{
	out_Color = vec4(sin(tex_Coord.s * 50.0), sin(tex_Coord.t * 50.0), 1.0, 1.0);
}
