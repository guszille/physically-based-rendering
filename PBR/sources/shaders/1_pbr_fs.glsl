#version 460 core

in vec3 ioWorldPos;
in vec3 ioNormal;
in vec2 ioTexCoords;

out vec4 oFragColor;

// Material parameters.
uniform  vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAO;

// IBL.
uniform samplerCube uIrradianceMap;

// Lights parameters.
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];

uniform vec3 uCameraPos;

const float PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a1 = roughness * roughness;
    float a2 = a1 * a1;
    float NdotH1 = max(dot(N, H), 0.0);
    float NdotH2 = NdotH1 * NdotH1;

    float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);

    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float numerator = NdotV;
    float denominator = NdotV * (1.0 - k) + k;

    return numerator / denominator;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float numerator = geometrySchlickGGX(NdotV, roughness);
    float denominator = geometrySchlickGGX(NdotL, roughness);

    return numerator * denominator;
}

vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 N = normalize(ioNormal);
    vec3 V = normalize(uCameraPos - ioWorldPos);

    // Calculate reflectance at normal incidence:
    //
    //  - if dia-electric (like plastic) use F0 of 0.04;
    //  - if it's a metal, use the albedo color as F0 (metallic workflow).
    //
    vec3 F0 = mix(vec3(0.04), uAlbedo, uMetallic);

    // Reflectance equation.
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < 4; ++i)
    {
        // Calculate per-light radiance.
        vec3  L = normalize(uLightPositions[i] - ioWorldPos);
        vec3  H = normalize(V + L);

        float lightDistance = length(uLightPositions[i] - ioWorldPos);
        float attenuation = 1.0 / (lightDistance * lightDistance);
        vec3  radiance = uLightColors[i] * attenuation;

        // Cook-Torrance BRDF.
        float NDF = distributionGGX(N, H, uRoughness);
        float G = geometrySmith(N, V, L, uRoughness);
        vec3  F = fresnelSchlick(F0, clamp(dot(H, V), 0.0, 1.0));
           
        vec3  numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent division by zero.
        vec3  specular = numerator / denominator;

        vec3  kS = F; // kS is equal to Fresnel.

        // For energy conservation, the diffuse and specular light can't be above 1.0 (unless the surface emits light);
        // To preserve this relationship the diffuse component (kD) should equal 1.0 - kS.
        //
        vec3  kD = vec3(1.0) - kS;

        // Multiply kD by the inverse metalness such that only non-metals have diffuse lighting, or
        // a linear blend if partly metal (pure metals have no diffuse light).
        //
        kD *= 1.0 - uMetallic;

        float NdotL = max(dot(N, L), 0.0); // Scale light by NdotL.

        // Note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again.
        //
        Lo += (kD * uAlbedo / PI + specular) * radiance * NdotL;
    }

    // Ambient light, old version...
    // vec3 ambient = vec3(0.03) * uAlbedo * uAO;

    // Ambient light, IBL approach.
    vec3 kS = fresnelSchlick(F0, max(dot(N, V), 0.0));
    vec3 kD = 1.0 - kS;

    kD *= 1.0 - uMetallic;
    
    vec3 irradiance = texture(uIrradianceMap, N).rgb;
    vec3 diffuse = irradiance * uAlbedo;
    vec3 ambient = (kD * diffuse) * uAO;

    // Final pixel color.
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0)); // HDR tonemapping.
    color = pow(color, vec3(1.0 / 2.2)); // Gamma correction.

    oFragColor = vec4(color, 1.0);
}
