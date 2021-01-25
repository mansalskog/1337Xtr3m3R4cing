#version 150

out vec3 normalVec;
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

	normalVec = in_Normal;

	tex_Coord = inTexCoord;
}
