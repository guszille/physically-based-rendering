#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 ioWorldPos;
out vec3 ioNormal;
out vec2 ioTexCoords;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

void main()
{
    ioWorldPos = vec3(uModel * vec4(aPos, 1.0));
    ioNormal = uNormalMatrix * aNormal;
    ioTexCoords = aTexCoords;

    gl_Position =  uProjection * uView * vec4(ioWorldPos, 1.0);
}
