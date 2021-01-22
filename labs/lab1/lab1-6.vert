#version 150

out vec3 vert_Color;

in vec3 in_Normal;
in vec3 in_Position;

uniform mat4 zMatrix;
uniform mat4 yMatrix;

void main(void)
{
	gl_Position = yMatrix * zMatrix * vec4(in_Position, 1.0);

	mat4 normalMatrix = inverse(transpose(yMatrix * zMatrix));
	vec4 transfNormal = normalMatrix * vec4(in_Normal, 1.0);
	float lightness = dot(transfNormal.xyz, vec3(0.2, 1.0, 0.2));
	vert_Color = vec3(1.0 - smoothstep(0.2, 0.8, lightness), 0.5, 0.5);
}
