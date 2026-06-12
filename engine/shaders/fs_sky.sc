$input v_skyDirection

#include <bgfx_shader.sh>

uniform vec4 u_lightDirection;
uniform vec4 u_skyHorizon;
uniform vec4 u_skyZenith;
uniform vec4 u_skySunGlow;

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
    vec3 direction = normalize(v_skyDirection);
    float skyBlend = saturate(direction.y * 0.5 + 0.5);
    vec3 skyColor = mix(u_skyHorizon.rgb, u_skyZenith.rgb, skyBlend);
    float sunAmount = pow(max(dot(direction, normalize(-u_lightDirection.xyz)), 0.0), 256.0);
    skyColor += u_skySunGlow.rgb * sunAmount * 8.0;
    gl_FragColor = vec4(skyColor, 1.0);
}
