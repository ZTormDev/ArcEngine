$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneTex, 0); // Scene color (original HDR scene)
SAMPLER2D(s_depthTex, 1); // Depth buffer
SAMPLER2D(s_gbufferNormal, 2); // View-space normal, packed [0,1]

uniform vec4 u_ssgiParams;       // x: screenWidth, y: screenHeight, z: maxDistance, w: intensity
uniform vec4 u_cameraProjParams; // x: tanHalfFovX, y: tanHalfFovY, z: nearPlane, w: farPlane

float getLinearDepth(vec2 uv)
{
    return texture2DLod(s_depthTex, uv, 0.0).r;
}

vec3 getViewPos(vec2 uv, float depth)
{
    return vec3((uv * 2.0 - 1.0) * u_cameraProjParams.xy * depth, -depth);
}

void main()
{
    vec2 uv = v_texcoord0;
    float depth = getLinearDepth(uv);

    // Skip sky/far plane
    if (depth > u_cameraProjParams.w - 5.0)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    vec3 viewPos = getViewPos(uv, depth);
    vec3 normal = texture2DLod(s_gbufferNormal, uv, 0.0).xyz * 2.0 - 1.0;
    normal = normalize(normal);

    // Generate pseudo-random rotation angle using pixel noise
    float noise = fract(sin(dot(uv * u_ssgiParams.xy, vec2(12.9898, 78.233))) * 43758.5453);
    float angle = noise * 6.2831853;
    float cosA = cos(angle);
    float sinA = sin(angle);

    // TBN Space alignment
    vec3 rvec = vec3(cosA, sinA, 0.0);
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    // 8-sample hemisphere direction distribution
    vec3 kernel[8];
    kernel[0] = vec3( 0.577,  0.577,  0.577);
    kernel[1] = vec3(-0.577,  0.577,  0.577);
    kernel[2] = vec3( 0.577, -0.577,  0.577);
    kernel[3] = vec3(-0.577, -0.577,  0.577);
    kernel[4] = vec3( 0.707,  0.000,  0.707);
    kernel[5] = vec3(-0.707,  0.000,  0.707);
    kernel[6] = vec3( 0.000,  0.707,  0.707);
    kernel[7] = vec3( 0.000, -0.707,  0.707);

    float maxDistance = u_ssgiParams.z; // e.g. 3.0
    float intensity = u_ssgiParams.w;   // e.g. 1.5
    float stepSize = maxDistance / 3.0; // 3 steps per ray
    float thickness = 0.5;

    vec3 indirectLight = vec3(0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    for (int r = 0; r < 8; ++r)
    {
        // Re-orient sample ray to normal space
        vec3 D = normalize(mul(tbn, kernel[r]));
        
        // Offset starting point slightly along normal to prevent self-intersection
        vec3 rayStart = viewPos + normal * 0.08;

        for (int s = 1; s <= 3; ++s)
        {
            float dist = stepSize * (float(s) - 0.5 * noise); // Jitter step distance to reduce banding
            vec3 currentPos = rayStart + D * dist;

            // Project to UV
            vec2 sampleUV;
            sampleUV.x = (currentPos.x / (-currentPos.z * u_cameraProjParams.x)) * 0.5 + 0.5;
            sampleUV.y = (currentPos.y / (-currentPos.z * u_cameraProjParams.y)) * 0.5 + 0.5;

            // Bounds check
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
                break;

            float sampleDepth = getLinearDepth(sampleUV);
            float depthDiff = -currentPos.z - sampleDepth;

            // Check if ray intersects geometry
            if (depthDiff >= 0.0 && depthDiff < thickness)
            {
                vec3 hitColor = texture2DLod(s_sceneTex, sampleUV, 0.0).rgb;

                // Cosine weighting (Lamberts law) & distance falloff
                float cosWeight = max(dot(normal, D), 0.0);
                float distFalloff = clamp(1.0 - (dist / maxDistance), 0.0, 1.0);
                float weight = cosWeight * distFalloff;

                // Avoid bleeding from extremely bright sky pixels or self-reflections
                float sampleLuminance = dot(hitColor, vec3(0.2126, 0.7152, 0.0722));
                if (sampleLuminance < 15.0) // Clamp super-bright emissive bleeding to avoid GI blowout
                {
                    indirectLight += hitColor * weight;
                    totalWeight += weight;
                }
                break;
            }
        }
    }

    vec3 finalSSGI = vec3(0.0, 0.0, 0.0);
    if (totalWeight > 0.0001)
    {
        finalSSGI = (indirectLight / totalWeight) * intensity;
    }

    // Edge fading for SSGI to prevent screen-boundary artifacts
    vec2 edgeFactor = smoothstep(vec2(0.0, 0.0), vec2(0.08, 0.08), uv) * 
                      smoothstep(vec2(0.0, 0.0), vec2(0.08, 0.08), 1.0 - uv);
    finalSSGI *= (edgeFactor.x * edgeFactor.y);

    // Safeguard values
    if (finalSSGI.x * 0.0 != 0.0 || finalSSGI.x < 0.0) finalSSGI.x = 0.0;
    if (finalSSGI.y * 0.0 != 0.0 || finalSSGI.y < 0.0) finalSSGI.y = 0.0;
    if (finalSSGI.z * 0.0 != 0.0 || finalSSGI.z < 0.0) finalSSGI.z = 0.0;

    gl_FragColor = vec4(finalSSGI, 1.0);
}
