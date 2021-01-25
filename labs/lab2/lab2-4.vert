#version 150

out vec3 vert_Color;
out vec2 tex_Coord;

in vec3 in_Normal;
in vec3 in_Position;
in vec2 inTexCoord;

uniform mat4 projection;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;

void main(void)
{
	mat4 totalMatrix = projection * viewMatrix * modelMatrix;
	gl_Position = totalMatrix * vec4(in_Position, 1.0);

	mat4 normalMatrix = inverse(transpose(totalMatrix));
	vec4 transfNormal = normalMatrix * vec4(in_Normal, 1.0);
	vec3 lightPos = vec3(0.5, 1.5, -0.5);
	float lightness = dot(transfNormal.xyz, lightPos);
	// vert_Color = in_Normal;
	vert_Color = vec3(smoothstep(0.2, 0.6, lightness), 0.5, 0.5);
	tex_Coord = inTexCoord;
}
