$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_currentTex, 0); // Scene Color (e.g. TAA resolved)
SAMPLER2D(s_depthTex, 1);   // Depth buffer

uniform vec4 u_dofParams; // x: focusDistance, y: focusRange, z: nearPlane, w: farPlane

#define NUM_DOF_SAMPLES 32

void main()
{
    vec2 uv = v_texcoord0;

    // Read NDC depth from buffer
    float ndcDepth = texture2D(s_depthTex, uv).r;

    // Reconstruct linear depth
    float near = u_dofParams.z;
    float far = u_dofParams.w;
    float depth = (near * far) / (far - ndcDepth * (far - near));

    // Calculate Circle of Confusion (CoC)
    float focusDist = u_dofParams.x;
    float focusRange = u_dofParams.y;
    float coc = clamp(abs(depth - focusDist) / focusRange, 0.0, 1.0);

    // Max radius in UV coordinates
    float maxRadius = 0.015;
    float radius = coc * maxRadius;

    vec3 colorAccum = vec3(0.0, 0.0, 0.0);
    float weightAccum = 0.0;

    // Sample center
    vec3 centerColor = texture2D(s_currentTex, uv).rgb;
    colorAccum += centerColor;
    weightAccum += 1.0;

    // Golden angle in radians
    float goldenAngle = 2.39996323;

    for (int i = 1; i < NUM_DOF_SAMPLES; ++i)
    {
        float theta = float(i) * goldenAngle;
        float r = sqrt(float(i) / float(NUM_DOF_SAMPLES));
        vec2 offset = vec2(cos(theta), sin(theta)) * r * radius;

        // Sample color and depth
        vec3 sampleColor = texture2D(s_currentTex, uv + offset).rgb;
        float sampleDepthNDC = texture2D(s_depthTex, uv + offset).r;
        float sampleDepth = (near * far) / (far - sampleDepthNDC * (far - near));
        float sampleCoc = clamp(abs(sampleDepth - focusDist) / focusRange, 0.0, 1.0);

        // Depth-dependent blending to prevent background color bleeding onto sharp foreground silhouettes
        float weight = 1.0;
        if (sampleDepth > depth)
        {
            // Background sample: scale weight by its own blur amount
            weight = sampleCoc;
        }

        colorAccum += sampleColor * weight;
        weightAccum += weight;
    }

    vec3 finalColor = colorAccum / max(weightAccum, 0.0001);
    gl_FragColor = vec4(finalColor, 1.0);
}
