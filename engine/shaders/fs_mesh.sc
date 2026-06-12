$input v_worldNormal, v_color0, v_worldPosition

#include <bgfx_shader.sh>

uniform vec4 u_lightDirection;
uniform vec4 u_lightColor;
uniform vec4 u_cameraData;
uniform vec4 u_ambientColor;
uniform vec4 u_tint;
uniform vec4 u_material;

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
    vec3 normal = normalize(v_worldNormal);
    vec3 viewDirection = normalize(u_cameraData.xyz - v_worldPosition);
    vec3 lightDirection = normalize(-u_lightDirection.xyz);
    vec3 halfway = normalize(viewDirection + lightDirection);

    vec3 albedo = max(v_color0.rgb * u_tint.rgb, vec3(0.0, 0.0, 0.0));
    float metallic = saturate(u_material.x);
    float roughness = clamp(u_material.y, 0.045, 1.0);
    float emissiveStrength = max(u_material.z, 0.0);
    float nDotL = max(dot(normal, lightDirection), 0.0);
    float nDotV = max(dot(normal, viewDirection), 0.0);

    vec3 f0 = mix(vec3(0.04, 0.04, 0.04), albedo, metallic);
    vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDirection), 0.0), f0);
    float normalDistribution = distributionGGX(normal, halfway, roughness);
    float geometry = geometrySmith(normal, viewDirection, lightDirection, roughness);
    vec3 specular = (normalDistribution * geometry * fresnel) / max(4.0 * nDotV * nDotL, 0.0001);

    vec3 diffuseEnergy = (vec3(1.0, 1.0, 1.0) - fresnel) * (1.0 - metallic);
    vec3 radiance = u_lightColor.rgb * u_lightColor.a;
    vec3 direct = (diffuseEnergy * albedo / 3.14159265 + specular) * radiance * nDotL;
    vec3 ambient = albedo * u_ambientColor.rgb * u_ambientColor.a * (1.0 - metallic);
    vec3 emissive = albedo * emissiveStrength;

    vec3 color = (direct + ambient + emissive) * max(u_cameraData.w, 0.001);
    color = acesTonemap(color);
    color = pow(color, vec3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    gl_FragColor = vec4(color, u_tint.a * v_color0.a);
}
