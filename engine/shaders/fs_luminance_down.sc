$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

uniform vec4 u_downParams; // x: passIndex (0.0 = log-lum, 1.0 = box average), y: texelSize, z: unused, w: unused

float getLuminance(vec3 color)
{
    // Rec. 709 luminance coefficients
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec2 uv = v_texcoord0;
    float texelSize = u_downParams.y;

    // Sample 4 bilinear coordinates centered in the quadrants
    vec2 offset = vec2(texelSize * 0.5, texelSize * 0.5);

    vec3 c0 = texture2D(s_texture, uv + vec2(-offset.x, -offset.y)).rgb;
    vec3 c1 = texture2D(s_texture, uv + vec2( offset.x, -offset.y)).rgb;
    vec3 c2 = texture2D(s_texture, uv + vec2(-offset.x,  offset.y)).rgb;
    vec3 c3 = texture2D(s_texture, uv + vec2( offset.x,  offset.y)).rgb;

    if (u_downParams.x < 0.5) // Pass 0: Compute average log-luminance
    {
        float l0 = log(max(getLuminance(c0), 0.0001));
        float l1 = log(max(getLuminance(c1), 0.0001));
        float l2 = log(max(getLuminance(c2), 0.0001));
        float l3 = log(max(getLuminance(c3), 0.0001));
        float avgLogL = (l0 + l1 + l2 + l3) * 0.25;
        gl_FragColor = vec4(avgLogL, avgLogL, avgLogL, 1.0);
    }
    else // Subsequent passes: Simple box average of the log-luminance values
    {
        float avgLogL = (c0.r + c1.r + c2.r + c3.r) * 0.25;
        gl_FragColor = vec4(avgLogL, avgLogL, avgLogL, 1.0);
    }
}
