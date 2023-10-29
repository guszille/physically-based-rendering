#version 460 core

in vec3 ioWorldPos;
in vec3 ioNormal;
in vec2 ioTexCoords;

out vec4 oFragColor;

// Material parameters.
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uAOMap;

// IBL.
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;
uniform sampler2D uBRDFLUTMap;

// Lights parameters.
uniform vec3 uLightPositions[4];
uniform vec3 uLightColors[4];

uniform vec3 uCameraPos;

const float PI = 3.14159265359;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(uNormalMap, ioTexCoords).xyz * 2.0 - 1.0;

    vec3 Q1 = dFdx(ioWorldPos);
    vec3 Q2 = dFdy(ioWorldPos);
    vec2 ST1 = dFdx(ioTexCoords);
    vec2 ST2 = dFdy(ioTexCoords);

    vec3 N = normalize(ioNormal);
    vec3 T = normalize(Q1 * ST2.t - Q2 * ST1.t);
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, -B, N);

    return normalize(TBN * tangentNormal);
}

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

vec3 fresnelSchlickRoughness(float cosTheta, float roughness, vec3 F0)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3  albedo = pow(texture(uAlbedoMap, ioTexCoords).rgb, vec3(2.2));
    vec3  normal = getNormalFromMap();
    float metallic = texture(uMetallicMap, ioTexCoords).r;
    float roughness = texture(uRoughnessMap, ioTexCoords).r;
    float ao = texture(uAOMap, ioTexCoords).r;

    vec3 V = normalize(uCameraPos - ioWorldPos);
    vec3 R = reflect(-V, normal);

    // Calculate reflectance at normal incidence:
    //
    //  - if dia-electric (like plastic) use F0 of 0.04;
    //  - if it's a metal, use the albedo color as F0 (metallic workflow).
    //
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

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
        float NDF = distributionGGX(normal, H, roughness);
        float G = geometrySmith(normal, V, L, roughness);
        vec3  F = fresnelSchlick(F0, clamp(dot(H, V), 0.0, 1.0));
           
        vec3  numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001; // + 0.0001 to prevent division by zero.
        vec3  specular = numerator / denominator;

        vec3  kS = F; // kS is equal to Fresnel.

        // For energy conservation, the diffuse and specular light can't be above 1.0 (unless the surface emits light);
        // To preserve this relationship the diffuse component (kD) should equal 1.0 - kS.
        //
        vec3  kD = vec3(1.0) - kS;

        // Multiply kD by the inverse metalness such that only non-metals have diffuse lighting, or
        // a linear blend if partly metal (pure metals have no diffuse light).
        //
        kD *= 1.0 - metallic;

        float NdotL = max(dot(normal, L), 0.0); // Scale light by NdotL.

        // Note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again.
        //
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient light, old version...
    // vec3 ambient = vec3(0.03) * albedo * ao;

    // Ambient light, IBL approach.
    vec3 F = fresnelSchlickRoughness(max(dot(normal, V), 0.0), roughness, F0);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;

    kD *= 1.0 - metallic;
    
    const float MAX_REFLECTION_LOD = 4.0;

    vec3 irradiance = texture(uIrradianceMap, normal).rgb;

    vec3 prefilteredColor = textureLod(uPrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 BRDF = texture(uBRDFLUTMap, vec2(max(dot(normal, V), 0.0), roughness)).rg;

    vec3 diffuse = irradiance * albedo;
    vec3 specular = prefilteredColor * (F * BRDF.x + BRDF.y);
    vec3 ambient = (kD * diffuse + specular) * ao;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0)); // HDR tonemapping.
    color = pow(color, vec3(1.0 / 2.2)); // Gamma correction.

    oFragColor = vec4(color, 1.0);
}
