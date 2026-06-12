$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_currentTex, 0);
SAMPLER2D(s_historyTex, 1);
SAMPLER2D(s_velocityTex, 2);

uniform vec4 u_taaParams; // x: feedback, y: texelWidth, z: texelHeight, w: unused
uniform vec4 u_taaJitter; // xy: current frame jitter, zw: previous frame jitter

float getLuminance(vec3 c)
{
    // Rec. 709 luminance
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

vec3 tonemap(vec3 c)
{
    // Reinhard tonemap to compress highlights and prevent high-contrast shimmering
    return c / (1.0 + getLuminance(c));
}

vec3 untonemap(vec3 c)
{
    return c / max(1.0 - getLuminance(c), 0.0001);
}

// 9-tap Catmull-Rom bicubic filter to sample history and keep it sharp
vec3 sampleTextureCatmullRom(sampler2D tex, vec2 uv, vec2 texSize)
{
    vec2 samplePos = uv * texSize;
    vec2 texelPos = floor(samplePos - 0.5) + 0.5;
    vec2 f = samplePos - texelPos;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12 = w1 + w2;
    vec2 tc12 = (texelPos + w2 / w12) / texSize;
    vec2 tc0 = (texelPos - 1.0) / texSize;
    vec2 tc3 = (texelPos + 2.0) / texSize;

    vec3 result = vec3(0.0, 0.0, 0.0);
    
    // Row 0
    result += texture2D(tex, vec2(tc0.x, tc0.y)).rgb * (w0.x * w0.y);
    result += texture2D(tex, vec2(tc12.x, tc0.y)).rgb * (w12.x * w0.y);
    result += texture2D(tex, vec2(tc3.x, tc0.y)).rgb * (w3.x * w0.y);

    // Row 1
    result += texture2D(tex, vec2(tc0.x, tc12.y)).rgb * (w0.x * w12.y);
    result += texture2D(tex, vec2(tc12.x, tc12.y)).rgb * (w12.x * w12.y);
    result += texture2D(tex, vec2(tc3.x, tc12.y)).rgb * (w3.x * w12.y);

    // Row 2
    result += texture2D(tex, vec2(tc0.x, tc3.y)).rgb * (w0.x * w3.y);
    result += texture2D(tex, vec2(tc12.x, tc3.y)).rgb * (w12.x * w3.y);
    result += texture2D(tex, vec2(tc3.x, tc3.y)).rgb * (w3.x * w3.y);

    return result;
}

// Clips history color towards the neighborhood center color to the AABB bounding box
vec3 clipToAABB(vec3 color, vec3 boxMin, vec3 boxMax)
{
    vec3 center = (boxMin + boxMax) * 0.5;
    vec3 extents = (boxMax - boxMin) * 0.5;
    vec3 v = color - center;
    vec3 absV = abs(v);

    if (all(lessThanEqual(absV, extents)))
    {
        return color;
    }
    else
    {
        vec3 t = extents / max(absV, vec3(0.0001, 0.0001, 0.0001));
        float minT = min(t.x, min(t.y, t.z));
        return center + v * minT;
    }
}

void main()
{
    vec2 uv = v_texcoord0;

    float dx = u_taaParams.y;
    float dy = u_taaParams.z;

    // 3x3 Velocity dilation: select the velocity vector with the largest magnitude
    // to ensure geometry silhouettes use the foreground object's velocity.
    vec2 velocity = vec2(0.0, 0.0);
    float maxLen = -1.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 v = texture2D(s_velocityTex, uv + vec2(float(x) * dx, float(y) * dy)).rg;
            float len = dot(v, v);
            if (len > maxLen)
            {
                maxLen = len;
                velocity = v;
            }
        }
    }

    // Reproject history coordinate (pure geometric reprojection since history is unjittered)
    vec2 historyUV = uv - velocity;

    // Unjitter the current frame sampling coordinates for the blend target
    vec2 currentUV = uv + u_taaJitter.xy;

    // Read current pixel
    vec3 current = texture2D(s_currentTex, currentUV).rgb;

    // Calculate mean and variance of 3x3 neighborhood of current frame in tonemapped space
    // centered around currentUV to align with the unjittered center color.
    vec3 m1 = vec3(0.0, 0.0, 0.0);
    vec3 m2 = vec3(0.0, 0.0, 0.0);

    for (int ny = -1; ny <= 1; ++ny)
    {
        for (int nx = -1; nx <= 1; ++nx)
        {
            vec3 c = texture2D(s_currentTex, currentUV + vec2(float(nx) * dx, float(ny) * dy)).rgb;
            c = tonemap(c);
            m1 += c;
            m2 += c * c;
        }
    }

    vec3 mean = m1 / 9.0;
    vec3 stdDev = sqrt(max(m2 / 9.0 - mean * mean, vec3(0.0, 0.0, 0.0)));

    // Variance bounding box clamp
    float gamma = 2.0; 
    vec3 boxMin = mean - gamma * stdDev;
    vec3 boxMax = mean + gamma * stdDev;

    // Sample reprojected history using high-quality Catmull-Rom filtering
    vec2 texSize = vec2(1.0 / dx, 1.0 / dy);
    vec3 history = sampleTextureCatmullRom(s_historyTex, historyUV, texSize);

    // Apply tonemap to history to perform clipping in the same space
    history = tonemap(history);

    // Clip history towards the neighborhood center (mean) to the local bounding box
    vec3 clippedHistory = clipToAABB(history, boxMin, boxMax);

    // Blend current and history in tonemapped space
    // Velocity-adaptive feedback: use higher feedback (e.g. 0.97) when static for max stability,
    // and lower feedback (e.g. 0.85) when moving to prevent ghosting.
    float feedback = u_taaParams.x;
    if (feedback > 0.0)
    {
        float speed = length(velocity);
        feedback = mix(0.97, 0.85, saturate(speed * 100.0));
    }

    // Discard history completely if UV coordinates are out of bounds
    if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0)
    {
        feedback = 0.0;
    }

    vec3 currentTonemapped = tonemap(current);
    vec3 resolved = mix(currentTonemapped, clippedHistory, feedback);

    // Convert back to linear space
    resolved = untonemap(resolved);

    gl_FragColor = vec4(resolved, 1.0);
}
