#version 130

in vec4 vPosition;
in vec4 vColor;
out vec4 color;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform int grey;

void main()
{
	gl_Position = Projection * ModelView * vPosition;

	if (grey == 1) {
		float c = (vColor.r + vColor.g + vColor.b) / 3;
		color = vec4(c, c, c, vColor.a);
	} else {
		color = vColor;
	}
}
