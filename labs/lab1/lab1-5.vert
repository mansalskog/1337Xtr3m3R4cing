#version 150

in vec3 in_Color;
out vec3 vert_Color;

in vec3 in_Position;

uniform mat4 zMatrix;
uniform mat4 yMatrix;

void main(void)
{
	gl_Position = yMatrix * zMatrix * vec4(in_Position, 1.0);
	vert_Color = in_Color;
}
