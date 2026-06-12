$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneTex, 0);
SAMPLER2D(s_bloomTex, 1);
uniform vec4 u_postParams; // x: exposure, y: bloomIntensity, z: reserved, w: reserved

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

    // Mode 0.0: Full HDR Post-processing (Exposure + ACES + Bloom + Gamma 2.2)
    vec3 bloomColor = texture2D(s_bloomTex, uv).rgb;
    vec3 color = sceneColor + bloomColor * u_postParams.y;
    color *= u_postParams.x;
    color = acesTonemap(color);
    color = pow(max(color, vec3(0.0, 0.0, 0.0)), vec3(1.0/2.2, 1.0/2.2, 1.0/2.2));

    gl_FragColor = vec4(color, 1.0);
}
