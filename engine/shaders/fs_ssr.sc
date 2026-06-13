$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneTex,     0); // Scene Color
SAMPLER2D(s_depthTex,     1); // Depth buffer
SAMPLER2D(s_velocityTex,  2); // Packed: xy=velocity, z=roughness, w=metallic
SAMPLER2D(s_gbufferNormal,3); // View-space normal, packed [0,1]
SAMPLER2D(s_bloomTex,     4); // Previous frame bloom downsample (half-res, blurred)

// x: screenWidth  y: screenHeight  z: maxDistance  w: stepLength
uniform vec4 u_ssrParams;
// x: tanHalfFovX  y: tanHalfFovY  z: nearPlane  w: farPlane
uniform vec4 u_cameraProjParams;

// ---------------------------------------------------------------------------
// Depth helpers
// ---------------------------------------------------------------------------
float sampleLinearDepth(vec2 uv)
{
    return texture2DLod(s_depthTex, uv, 0.0).r;
}

// ---------------------------------------------------------------------------
// Reconstruct view-space position from UV and depth.
// BGFX: camera looks down -Z, geometry at negative Z.
// ---------------------------------------------------------------------------
vec3 viewPosFromUV(vec2 uv, float eyeDepth)
{
    // UV Y is flipped vs NDC Y in DirectX/BGFX
    float ndcX =  (uv.x * 2.0 - 1.0);
    float ndcY = -(uv.y * 2.0 - 1.0);
    return vec3(
        ndcX * u_cameraProjParams.x * eyeDepth,
        ndcY * u_cameraProjParams.y * eyeDepth,
        -eyeDepth
    );
}

// ---------------------------------------------------------------------------
// Project view-space point back to UV.
// ---------------------------------------------------------------------------
vec2 projectToUV(vec3 vsPos)
{
    float invZ = 1.0 / (-vsPos.z);
    float ndcX =  vsPos.x * invZ / u_cameraProjParams.x;
    float ndcY =  vsPos.y * invZ / u_cameraProjParams.y;
    return vec2(
         ndcX * 0.5 + 0.5,
        -ndcY * 0.5 + 0.5
    );
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
void main()
{
    vec2 uv = v_texcoord0;

    // Read packed material
    vec4 velMat    = texture2DLod(s_velocityTex, uv, 0.0);
    float roughness = velMat.z;
    float metallic  = velMat.w;

    // Skip semi-rough / non-reflective pixels early
    if (roughness > 0.5)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    // Skip sky / far plane (using linear depth threshold close to far plane)
    float eyeDepth = sampleLinearDepth(uv);
    if (eyeDepth >= u_cameraProjParams.w - 2.0)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    vec3 vsPos     = viewPosFromUV(uv, eyeDepth);

    // Read the exact GBuffer normal — no derivative approximation needed
    vec3 vsNorm = texture2DLod(s_gbufferNormal, uv, 0.0).xyz * 2.0 - 1.0;
    vsNorm = normalize(vsNorm);

    vec3 vsDir = normalize(vsPos);       // camera → surface
    vec3 R     = reflect(vsDir, vsNorm); // reflection direction in view-space

    // ---------------------------------------------------------------------------
    // Ray march in view space
    // ---------------------------------------------------------------------------
    float maxDistance = u_ssrParams.z;
    float stepSize    = u_ssrParams.w;
    const int maxSteps = 32;
    const int binSteps = 4;
    float thickness = 0.3;

    // Interleaved gradient noise — low discrepancy, stable
    float noise = fract(52.9829189 * fract(dot(uv * u_ssrParams.xy, vec2(0.06711056, 0.00583715))));
    // Start slightly above surface along the GBuffer normal to avoid self-intersection
    vec3 rayStart = vsPos + vsNorm * (eyeDepth * 0.002 + 0.02);

    vec3 hitPos = vec3(0.0, 0.0, 0.0);
    vec2 hitUV  = vec2(-1.0, -1.0);
    bool hitFound = false;

    for (int i = 0; i < maxSteps; ++i)
    {
        float dist  = (float(i) + noise) * stepSize;
        if (dist > maxDistance) break;

        vec3 curPos = rayStart + R * dist;

        // Must stay in front of the near plane
        if (curPos.z >= -u_cameraProjParams.z) break;

        vec2 sUV = projectToUV(curPos);
        if (sUV.x < 0.0 || sUV.x > 1.0 || sUV.y < 0.0 || sUV.y > 1.0) break;

        float sceneDepth = sampleLinearDepth(sUV);
        float rayDepth   = -curPos.z;
        float depthDiff  = rayDepth - sceneDepth;

        if (depthDiff > 0.0 && depthDiff < thickness)
        {
            // Binary search refinement
            float lo = max(0.0, dist - stepSize);
            float hi = dist;

            for (int j = 0; j < binSteps; ++j)
            {
                float mid    = (lo + hi) * 0.5;
                vec3  midPos = rayStart + R * mid;
                vec2  midUV  = projectToUV(midPos);
                float midScene = sampleLinearDepth(midUV);
                float midDiff  = -midPos.z - midScene;

                if (midDiff > 0.0)
                {
                    hi     = mid;
                    hitPos = midPos;
                    hitUV  = midUV;
                }
                else
                {
                    lo = mid;
                }
            }

            if (hitUV.x >= 0.0)
            {
                float finalDiff = abs(-hitPos.z - sampleLinearDepth(hitUV));
                if (finalDiff < thickness)
                    hitFound = true;
            }
            break;
        }
    }

    if (!hitFound)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    vec3 reflectedColor = texture2DLod(s_sceneTex, hitUV, 0.0).rgb;
    reflectedColor = min(reflectedColor, vec3(8.0, 8.0, 8.0));

    // Screen-edge fade (wider zone = gentler transition)
    vec2 edgeFade = smoothstep(vec2(0.0, 0.0), vec2(0.15, 0.15), hitUV)
                  * smoothstep(vec2(0.0, 0.0), vec2(0.15, 0.15), 1.0 - hitUV);
    float screenFade = edgeFade.x * edgeFade.y;

    // Distance fade (quadratic)
    float distFade = clamp(1.0 - length(hitPos - vsPos) / maxDistance, 0.0, 1.0);
    distFade = distFade * distFade;

    // Roughness fade
    float roughnessFade = clamp(1.0 - roughness / 0.5, 0.0, 1.0);
    roughnessFade = roughnessFade * roughnessFade;

    // Grazing angle fade
    float NdotV = clamp(dot(vsNorm, -vsDir), 0.0, 1.0);
    float grazingFade = smoothstep(0.0, 0.25, NdotV);

    float combinedFade = screenFade * distFade * roughnessFade * grazingFade;

    // Fresnel-modulated specular reflection
    vec3 baseColor = texture2DLod(s_sceneTex, uv, 0.0).rgb;
    vec3 f0        = mix(vec3(0.04, 0.04, 0.04), baseColor, metallic);
    float cosTheta = clamp(dot(vsNorm, -vsDir), 0.0, 1.0);
    vec3  F        = f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);

    vec3 finalSSR = F * reflectedColor * combinedFade;

    // Bloom contribution: additive glow from emissive/bright objects in the reflection.
    // Still needs roughness + grazing fades so rough surfaces don't get ghost bloom halos.
    vec3 reflectedBloom = texture2DLod(s_bloomTex, hitUV, 0.0).rgb;
    finalSSR += reflectedBloom * screenFade * distFade * roughnessFade * grazingFade * 0.5;

    finalSSR = clamp(finalSSR, vec3(0.0, 0.0, 0.0), vec3(32.0, 32.0, 32.0));

    gl_FragColor = vec4(finalSSR, 1.0);
}
