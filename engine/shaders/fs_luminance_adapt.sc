$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_currentTex, 0); // Current 1x1 log-luminance
SAMPLER2D(s_historyTex, 1); // Previous 1x1 adapted linear luminance

uniform vec4 u_adaptationParams; // x: dt, y: speedUp, z: speedDown, w: unused

void main()
{
    // Extract current luminance (convert back from log space)
    float currentLogL = texture2D(s_currentTex, vec2(0.5, 0.5)).r;
    float currentL = exp(currentLogL);

    // Extract previous adapted luminance
    float prevL = texture2D(s_historyTex, vec2(0.5, 0.5)).r;

    // Temporal smoothing: adapt faster when moving into bright environments vs dark environments
    float dt = u_adaptationParams.x;
    float speed = (currentL > prevL) ? u_adaptationParams.y : u_adaptationParams.z;

    float adaptedL = prevL + (currentL - prevL) * (1.0 - exp(-dt * speed));

    // Clamp adapted luminance to a realistic range to prevent complete black/whiteouts
    adaptedL = clamp(adaptedL, 0.05, 10.0);

    gl_FragColor = vec4(adaptedL, adaptedL, adaptedL, 1.0);
}
