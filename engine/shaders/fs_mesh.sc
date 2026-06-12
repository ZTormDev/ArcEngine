$input v_worldNormal, v_color0, v_worldPosition, v_texcoord0, v_worldTangent

#include <bgfx_shader.sh>

SAMPLER2D(s_albedo, 0);
SAMPLER2D(s_normal, 1);
SAMPLER2D(s_metallicRoughness, 2);
SAMPLER2D(s_ao, 3);
SAMPLER2D(s_emissive, 4);
SAMPLER2DSHADOW(s_shadowMap, 5);

uniform vec4 u_lightDirection;
uniform vec4 u_lightColor;
uniform vec4 u_cameraData;
uniform vec4 u_ambientColor;
uniform vec4 u_tint;
uniform vec4 u_material;

uniform mat4 u_shadowMtx[4];
uniform vec4 u_cameraForward;
uniform vec4 u_cascadeSplits;

vec3 fresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

float distributionGGX(vec3 normal, vec3 halfway, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(normal, halfway), 0.0);
    float nDotH2 = nDotH * nDotH;
    float denom = nDotH2 * (a2 - 1.0) + 1.0;
    return a2 / max(3.14159265 * denom * denom, 0.0001);
}

float geometrySchlickGGX(float nDotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return nDotV / max(nDotV * (1.0 - k) + k, 0.0001);
}

float geometrySmith(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness)
{
    float nDotV = max(dot(normal, viewDirection), 0.0);
    float nDotL = max(dot(normal, lightDirection), 0.0);
    return geometrySchlickGGX(nDotV, roughness) * geometrySchlickGGX(nDotL, roughness);
}

vec3 acesTonemap(vec3 color)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

void main()
{
    vec3 vertexNormal = normalize(v_worldNormal);
    vec3 tangent = normalize(v_worldTangent.xyz);
    vec3 bitangent = normalize(cross(vertexNormal, tangent) * v_worldTangent.w);
    vec3 normalSample = texture2D(s_normal, v_texcoord0).xyz * 2.0 - 1.0;
    vec3 normal = normalize(tangent * normalSample.x + bitangent * normalSample.y + vertexNormal * normalSample.z);
    vec3 viewDirection = normalize(u_cameraData.xyz - v_worldPosition);
    vec3 lightDirection = normalize(-u_lightDirection.xyz);
    vec3 halfway = normalize(viewDirection + lightDirection);

    vec4 albedoSample = texture2D(s_albedo, v_texcoord0);
    vec4 metallicRoughnessSample = texture2D(s_metallicRoughness, v_texcoord0);
    float ao = texture2D(s_ao, v_texcoord0).r;
    vec3 emissiveSample = texture2D(s_emissive, v_texcoord0).rgb;

    vec3 albedo = max(v_color0.rgb * u_tint.rgb * albedoSample.rgb, vec3(0.0, 0.0, 0.0));
    float metallic = saturate(u_material.x * metallicRoughnessSample.b);
    float roughness = clamp(u_material.y * metallicRoughnessSample.g, 0.045, 1.0);
    float emissiveStrength = max(u_material.z, 0.0);
    float nDotL = max(dot(normal, lightDirection), 0.0);
    float nDotV = max(dot(normal, viewDirection), 0.0);

    vec3 f0 = mix(vec3(0.04, 0.04, 0.04), albedo, metallic);
    vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDirection), 0.0), f0);
    float normalDistribution = distributionGGX(normal, halfway, roughness);
    float geometry = geometrySmith(normal, viewDirection, lightDirection, roughness);
    vec3 specular = (normalDistribution * geometry * fresnel) / max(4.0 * nDotV * nDotL, 0.0001);

    // --- Shadow Calculation ---
    float depth = dot(v_worldPosition - u_cameraData.xyz, u_cameraForward.xyz);
    int cascadeIndex = 0;
    if (depth > u_cascadeSplits.x) {
        cascadeIndex = 1;
    }
    if (depth > u_cascadeSplits.y) {
        cascadeIndex = 2;
    }
    if (depth > u_cascadeSplits.z) {
        cascadeIndex = 3;
    }

    mat4 shadowMtx = u_shadowMtx[0];
    if (cascadeIndex == 1) {
        shadowMtx = u_shadowMtx[1];
    } else if (cascadeIndex == 2) {
        shadowMtx = u_shadowMtx[2];
    } else if (cascadeIndex == 3) {
        shadowMtx = u_shadowMtx[3];
    }

    float biasScale = 1.0;
    if (cascadeIndex == 1) {
        biasScale = 3.0;
    } else if (cascadeIndex == 2) {
        biasScale = 7.5;
    } else if (cascadeIndex == 3) {
        biasScale = 16.0;
    }

    float geoNDotL = max(dot(vertexNormal, lightDirection), 0.0);

    // Apply dynamic normal offset bias scaled by the cascade's world-space texel size and geometric slope
    float normalOffsetFactor = clamp(0.05 * (1.0 - geoNDotL), 0.01, 0.06);
    float normalOffset = normalOffsetFactor * biasScale;
    vec3 biasedWorldPos = v_worldPosition + vertexNormal * normalOffset;

    vec4 shadowProj = mul(shadowMtx, vec4(biasedWorldPos, 1.0));
    vec3 shadowCoord = shadowProj.xyz / shadowProj.w;

    // Check if the coordinate is inside the current cascade's projection box bounds
    bool inside = shadowCoord.x >= 0.0 && shadowCoord.x <= 1.0 &&
                  shadowCoord.y >= 0.0 && shadowCoord.y <= 1.0 &&
                  shadowCoord.z >= 0.0 && shadowCoord.z <= 1.0;

    float shadow = 1.0;
    if (inside)
    {
        vec2 offset = vec2(0.0, 0.0);
        if (cascadeIndex == 1) {
            offset = vec2(0.5, 0.0);
        } else if (cascadeIndex == 2) {
            offset = vec2(0.0, 0.5);
        } else if (cascadeIndex == 3) {
            offset = vec2(0.5, 0.5);
        }
        shadowCoord.xy = shadowCoord.xy * 0.5 + offset;

        // Use a dynamic slope-scaled depth bias based on geometric normal to prevent acne at grazing angles
        float bias = clamp(0.0005 * (1.0 - geoNDotL), 0.0001, 0.0012);
        float compareDepth = shadowCoord.z - bias;

        // 16 Poisson Disk samples declared cross-platform using ARRAY_BEGIN/ARRAY_END
        ARRAY_BEGIN(vec2, poissonDisk, 16)
            vec2(-0.94201624, -0.39906216),
            vec2(0.94558609, -0.76890725),
            vec2(-0.094184101, -0.92938870),
            vec2(0.34495938, 0.29387760),
            vec2(-0.91588581, 0.45771432),
            vec2(-0.81544232, -0.87912464),
            vec2(-0.38208752, 0.27676845),
            vec2(0.97484398, 0.75648379),
            vec2(0.44323325, -0.97511554),
            vec2(0.53742981, -0.47373420),
            vec2(-0.26496911, -0.41893023),
            vec2(0.79197514, 0.19090188),
            vec2(-0.24188840, 0.99792207),
            vec2(-0.81409955, 0.91437590),
            vec2(0.19984126, 0.78641367),
            vec2(0.14701631, -0.21163693)
        ARRAY_END();

        // Interleaved Gradient Noise for pseudo-random per-pixel rotation
        vec3 ignMagic = vec3(0.06711056, 0.00583715, 52.9829189);
        float ign = fract(ignMagic.z * fract(dot(gl_FragCoord.xy, ignMagic.xy)));
        float angle = ign * 6.28318530718;
        float s = sin(angle);
        float c = cos(angle);

        float texelSize = 1.0 / 8192.0;
        float filterRadius = 3.0 * texelSize;
        if (cascadeIndex == 1) {
            filterRadius = 2.5 * texelSize;
        } else if (cascadeIndex == 2) {
            filterRadius = 2.0 * texelSize;
        } else if (cascadeIndex == 3) {
            filterRadius = 1.5 * texelSize;
        }

        shadow = 0.0;
        vec2 minBound = offset;
        vec2 maxBound = offset + vec2(0.5, 0.5);
        for (int i = 0; i < 16; ++i)
        {
            vec2 offsetDir = poissonDisk[i];
            vec2 rotatedOffset = vec2(offsetDir.x * c - offsetDir.y * s, offsetDir.x * s + offsetDir.y * c);
            vec2 sampleCoord = clamp(shadowCoord.xy + rotatedOffset * filterRadius, minBound, maxBound);
            shadow += shadow2D(s_shadowMap, vec3(sampleCoord, compareDepth));
        }
        shadow /= 16.0;
    }

    // Fade out shadows smoothly beyond the last cascade split
    if (depth > u_cascadeSplits.w)
    {
        float fade = saturate((depth - u_cascadeSplits.w) / 10.0);
        shadow = mix(shadow, 1.0, fade);
    }

    vec3 diffuseEnergy = (vec3(1.0, 1.0, 1.0) - fresnel) * (1.0 - metallic);
    vec3 radiance = u_lightColor.rgb * u_lightColor.a;
    vec3 direct = (diffuseEnergy * albedo / 3.14159265 + specular) * radiance * nDotL * shadow;
    vec3 ambient = albedo * u_ambientColor.rgb * u_ambientColor.a * (1.0 - metallic) * ao;
    vec3 emissive = albedo * emissiveSample * emissiveStrength;

    vec3 color = (direct + ambient + emissive);
    gl_FragColor = vec4(color, u_tint.a * v_color0.a * albedoSample.a);
}
