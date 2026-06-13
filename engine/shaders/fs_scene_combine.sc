$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_sceneTex, 0); // HDR Scene Color
SAMPLER2D(s_ssaoTex, 1);  // Bilateral Blurred SSAO
SAMPLER2D(s_ssrTex, 2);   // Screen Space Reflections
SAMPLER2D(s_ssgiTex, 3);  // Screen Space Global Illumination

uniform vec4 u_combineParams; // x: ssaoEnabled, y: ssrEnabled, z: ssgiEnabled, w: unused

void main()
{
    vec2 uv = v_texcoord0;
    vec3 sceneColor = texture2D(s_sceneTex, uv).rgb;
    float ssao = u_combineParams.x > 0.5 ? texture2D(s_ssaoTex, uv).r : 1.0;
    vec3 ssr = u_combineParams.y > 0.5 ? texture2D(s_ssrTex, uv).rgb : vec3(0.0, 0.0, 0.0);
    vec3 ssgi = u_combineParams.z > 0.5 ? texture2D(s_ssgiTex, uv).rgb : vec3(0.0, 0.0, 0.0);

    // Apply SSAO. Mask out SSAO in extremely bright/emissive areas to prevent darkening glowing lights.
    float brightness = max(sceneColor.r, max(sceneColor.g, sceneColor.b));
    float ssaoMask = clamp(1.0 - brightness, 0.0, 1.0);
    vec3 ambientOccludedColor = sceneColor * mix(1.0, ssao, ssaoMask);

    // Accumulate direct/ambient + SSGI (indirect diffuse) + SSR (specular reflections)
    vec3 finalColor = ambientOccludedColor + ssgi + ssr;

    // Safeguard values
    if (finalColor.x * 0.0 != 0.0 || finalColor.x < 0.0) finalColor.x = 0.0;
    if (finalColor.y * 0.0 != 0.0 || finalColor.y < 0.0) finalColor.y = 0.0;
    if (finalColor.z * 0.0 != 0.0 || finalColor.z < 0.0) finalColor.z = 0.0;

    gl_FragColor = vec4(finalColor, 1.0);
}
