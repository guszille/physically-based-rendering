#version 460 core

layout (location = 0) in vec3 aPos;

out vec3 ioWorldPos;

uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    ioWorldPos = aPos;

    gl_Position =  uProjection * uView * vec4(ioWorldPos, 1.0);
}
