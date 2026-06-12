$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_currentTex, 0);
SAMPLER2D(s_velocityTex, 1);

uniform vec4 u_motionBlurParams; // x: blurScale, y: maxVelocityClamp, z: unused, w: unused

#define NUM_MB_SAMPLES 12

void main()
{
    vec2 uv = v_texcoord0;

    // Sample velocity vector
    vec2 velocity = texture2D(s_velocityTex, uv).rg;

    // Apply scale and clamp to avoid extreme/distracting streaks
    velocity *= u_motionBlurParams.x;
    float speed = length(velocity);
    if (speed > u_motionBlurParams.y)
    {
        velocity = (velocity / speed) * u_motionBlurParams.y;
    }

    vec3 accumColor = vec3(0.0, 0.0, 0.0);

    // Perform directional gather centered around the current pixel
    for (int i = 0; i < NUM_MB_SAMPLES; ++i)
    {
        float t = (float(i) / float(NUM_MB_SAMPLES - 1)) - 0.5;
        vec2 offset = velocity * t;
        accumColor += texture2D(s_currentTex, uv + offset).rgb;
    }

    vec3 finalColor = accumColor / float(NUM_MB_SAMPLES);
    gl_FragColor = vec4(finalColor, 1.0);
}
