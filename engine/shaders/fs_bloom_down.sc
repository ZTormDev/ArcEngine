$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
uniform vec4 u_downParams; // x: threshold, y: texelSizeX, z: texelSizeY, w: passIndex

vec3 prefilter(vec3 color, float threshold)
{
    float brightness = max(color.r, max(color.g, color.b));
    float knee = 0.1; // soft knee width
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.0001);
    float contribution = max(soft, brightness - threshold) / max(brightness, 0.0001);
    return color * contribution;
}

void main()
{
    vec2 uv = v_texcoord0;
    float x = u_downParams.y;
    float y = u_downParams.z;

    // 13-tap box downsample filter
    vec3 a = texture2D(s_texture, uv + vec2(-2.0*x,  2.0*y)).rgb;
    vec3 b = texture2D(s_texture, uv + vec2( 0.0,    2.0*y)).rgb;
    vec3 c = texture2D(s_texture, uv + vec2( 2.0*x,  2.0*y)).rgb;

    vec3 d = texture2D(s_texture, uv + vec2(-1.0*x,  1.0*y)).rgb;
    vec3 e = texture2D(s_texture, uv + vec2( 1.0*x,  1.0*y)).rgb;

    vec3 f = texture2D(s_texture, uv + vec2(-2.0*x,  0.0)).rgb;
    vec3 g = texture2D(s_texture, uv + vec2( 0.0,    0.0)).rgb;
    vec3 h = texture2D(s_texture, uv + vec2( 2.0*x,  0.0)).rgb;

    vec3 i = texture2D(s_texture, uv + vec2(-1.0*x, -1.0*y)).rgb;
    vec3 j = texture2D(s_texture, uv + vec2( 1.0*x, -1.0*y)).rgb;

    vec3 k = texture2D(s_texture, uv + vec2(-2.0*x, -2.0*y)).rgb;
    vec3 l = texture2D(s_texture, uv + vec2( 0.0,   -2.0*y)).rgb;
    vec3 m = texture2D(s_texture, uv + vec2( 2.0*x, -2.0*y)).rgb;

    vec3 color = (a+c+k+m)*0.03125 + (b+f+h+l)*0.0625 + (d+e+i+j)*0.125 + g*0.125;

    // Apply prefilter thresholding only on the first downsample pass (passIndex == 0)
    if (u_downParams.w < 0.5)
    {
        color = prefilter(color, u_downParams.x);
    }

    gl_FragColor = vec4(color, 1.0);
}
