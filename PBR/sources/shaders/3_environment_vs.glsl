#version 460 core

layout (location = 0) in vec3 aPos;

out vec3 ioWorldPos;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    ioWorldPos = aPos;

	mat4 rotateView = mat4(mat3(uView));
	vec4 clipPos = uProjection * rotateView * vec4(ioWorldPos, 1.0);

	gl_Position = clipPos.xyww;
}
