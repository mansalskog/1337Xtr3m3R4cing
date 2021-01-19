#version 150

out vec4 out_Color;

uniform vec4 tri_Color;

void main(void)
{
	out_Color = vec4(tri_Color.xy, abs(sin(gl_FragCoord.x / 200.0)), 1.0);
	// vec4(0.5, abs(cos(gl_FragCoord.y / 200.0)), abs(sin(gl_FragCoord.x / 200.0)), 1.0);
}
