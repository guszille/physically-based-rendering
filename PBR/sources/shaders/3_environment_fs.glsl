#version 330 core

in vec3 ioWorldPos;

out vec4 oFragColor;

uniform samplerCube uEnvironmentMap;

void main()
{
    vec3 environmentColor = texture(uEnvironmentMap, ioWorldPos).rgb;
    
    // HDR tonemapping and gamma correction.
    environmentColor = environmentColor / (environmentColor + vec3(1.0));
    environmentColor = pow(environmentColor, vec3(1.0 / 2.2));
    
    oFragColor = vec4(environmentColor, 1.0);
}
