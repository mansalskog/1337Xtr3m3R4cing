#version 150

out vec4 out_Color;
in vec3 middle_Color;

void main(void)
{
	out_Color = vec4(middle_Color, 1.0);
}
