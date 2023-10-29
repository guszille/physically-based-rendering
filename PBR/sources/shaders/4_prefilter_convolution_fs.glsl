#version 460 core

in vec3 ioWorldPos;

out vec4 oFragColor;

uniform samplerCube uEnvironmentMap;
uniform float uRoughness;

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


// Efficient "VanDerCorpus" calculation.
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
//
float radicalInverseVDC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), radicalInverseVDC(i));
}

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	// From spherical coordinates to cartesian coordinates (halfway vector).
	vec3 H;

	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	// From tangent-space vector (H) to world-space sample vector.
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;

	return normalize(sampleVec);
}

void main()
{
    vec3 N = normalize(ioWorldPos);
    
    // Make the simplifying assumption that V equals R equals the normal.
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;

    vec3 prefilteredColor = vec3(0.0);
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // Generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSampleGGX(Xi, N, uRoughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);

        if(NdotL > 0.0)
        {
            // Sample from the environment's mip level based on roughness/pdf.
            float D = distributionGGX(N, H, uRoughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float resolution = 512.0; // Resolution of source cubemap (per face).
            float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = uRoughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
            prefilteredColor += textureLod(uEnvironmentMap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    oFragColor = vec4(prefilteredColor, 1.0);
}
