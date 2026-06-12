$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneTex, 0);
SAMPLER2D(s_bloomTex, 1);
SAMPLER2D(s_luminanceTex, 2);
uniform vec4 u_postParams; // x: exposure, y: bloomIntensity, z: postProcessMode, w: autoExposureEnabled

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
    vec2 uv = v_texcoord0;
    vec3 sceneColor = texture2D(s_sceneTex, uv).rgb;

    // Mode 1.0: Raw Bypass (no gamma, no tonemap, no exposure, no bloom)
    if (u_postParams.z > 0.5 && u_postParams.z < 1.5)
    {
        gl_FragColor = vec4(sceneColor, 1.0);
        return;
    }

    // Mode 2.0: LDR (Gamma correction only, no exposure, no bloom, no ACES tonemapping)
    if (u_postParams.z > 1.5)
    {
        vec3 color = pow(max(sceneColor, vec3(0.0, 0.0, 0.0)), vec3(1.0/2.2, 1.0/2.2, 1.0/2.2));
        gl_FragColor = vec4(color, 1.0);
        return;
    }

    // Mode 0.0: Full HDR Post-processing (Exposure + ACES/Tonemap + Bloom + Gamma 2.2)
    vec3 bloomColor = texture2D(s_bloomTex, uv).rgb;
    vec3 color = sceneColor + bloomColor * u_postParams.y;

    if (color.x * 0.0 != 0.0 || color.x < 0.0) color.x = 0.0;
    if (color.y * 0.0 != 0.0 || color.y < 0.0) color.y = 0.0;
    if (color.z * 0.0 != 0.0 || color.z < 0.0) color.z = 0.0;
    color = clamp(color, vec3(0.0, 0.0, 0.0), vec3(65000.0, 65000.0, 65000.0));

    float exposure = u_postParams.x;
    float wVal = u_postParams.w;
    bool autoExposureActive = (wVal > 0.5 && wVal < 1.5) || (wVal > 2.5);
    bool tonemapActive = (wVal > 1.5);

    if (autoExposureActive)
    {
        float avgL = texture2D(s_luminanceTex, vec2(0.5, 0.5)).r;
        exposure = 0.22 / max(avgL, 0.001);
    }

    color *= exposure;

    if (tonemapActive)
    {
        color = acesTonemap(color);
    }
    else
    {
        color = saturate(color);
    }

    color = pow(max(color, vec3(0.0, 0.0, 0.0)), vec3(1.0/2.2, 1.0/2.2, 1.0/2.2));

    gl_FragColor = vec4(color, 1.0);
}

