#version 460 core

in vec3 ioWorldPos;

out vec4 oFragColor;

uniform sampler2D uEquirectangularMap;

const vec2 invatan = vec2(0.1591, 0.3183); // Inverse of "atan".

vec2 sampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));

    uv *= invatan;
    uv += 0.5;

    return uv;
}

void main()
{
    vec2 uv = sampleSphericalMap(normalize(ioWorldPos));
    vec3 color = texture(uEquirectangularMap, uv).rgb;
    
    oFragColor = vec4(color, 1.0);
}
