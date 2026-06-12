$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_lowTex, 0);
SAMPLER2D(s_highTex, 1);
uniform vec4 u_upParams; // x: filterRadius, y: texelSizeX, z: texelSizeY, w: blendWeight

void main()
{
    vec2 uv = v_texcoord0;
    float r = u_upParams.x;
    float x = u_upParams.y * r;
    float y = u_upParams.z * r;

    // 9-tap bilinear tent upsample filter
    vec3 a = texture2D(s_lowTex, uv + vec2(-x,  y)).rgb;
    vec3 b = texture2D(s_lowTex, uv + vec2( 0.0, y)).rgb;
    vec3 c = texture2D(s_lowTex, uv + vec2( x,  y)).rgb;

    vec3 d = texture2D(s_lowTex, uv + vec2(-x,  0.0)).rgb;
    vec3 e = texture2D(s_lowTex, uv + vec2( 0.0, 0.0)).rgb;
    vec3 f = texture2D(s_lowTex, uv + vec2( x,  0.0)).rgb;

    vec3 g = texture2D(s_lowTex, uv + vec2(-x, -y)).rgb;
    vec3 h = texture2D(s_lowTex, uv + vec2( 0.0,-y)).rgb;
    vec3 i = texture2D(s_lowTex, uv + vec2( x, -y)).rgb;

    vec3 lowColor = e*0.25 + (b+d+f+h)*0.125 + (a+c+g+i)*0.0625;
    vec3 highColor = texture2D(s_highTex, uv).rgb;

    // Additive blend of low-resolution blurred mip with high-resolution details
    vec3 color = highColor + lowColor * u_upParams.w;

    gl_FragColor = vec4(color, 1.0);
}
