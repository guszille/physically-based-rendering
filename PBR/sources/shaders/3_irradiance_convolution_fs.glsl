#version 460 core

in vec3 ioWorldPos;

out vec4 oFragColor;

uniform samplerCube uEnvironmentMap;

const float PI = 3.14159265359;

void main()
{
	// The world vector acts as the normal of a tangent surface from the origin, aligned to ioWorldPos.
    // Given this normal, calculate all incoming radiance of the environment.
    // The result of this radiance is the radiance of light coming from -Normal direction, which is what we use in the PBR shader to sample irradiance.
    //
    vec3 normal = normalize(ioWorldPos);
    vec3 irradiance = vec3(0.0);   
    
    // Tangent space calculation from origin point.
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up         = normalize(cross(normal, right));
       
    float sampleDelta = 0.025;
    float nSamples = 0.0;

    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta)); // Spherical to cartesian (in tangent space).
            vec3 worldSample = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; // Tangent space to world.

            irradiance += texture(uEnvironmentMap, worldSample).rgb * cos(theta) * sin(theta);

            nSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nSamples));
    
    oFragColor = vec4(irradiance, 1.0);
}
