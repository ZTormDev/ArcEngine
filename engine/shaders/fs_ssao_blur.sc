$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_ssaoTex, 0);
SAMPLER2D(s_depthTex, 1);

uniform vec4 u_ssaoParams;       // x: screenWidth, y: screenHeight, z: radius, w: bias
uniform vec4 u_cameraProjParams; // x: tanHalfFovX, y: tanHalfFovY, z: nearPlane, w: farPlane

float getLinearDepth(vec2 uv)
{
    float ndcDepth = texture2D(s_depthTex, uv).r;
    float near = u_cameraProjParams.z;
    float far = u_cameraProjParams.w;
    return (near * far) / (far - ndcDepth * (far - near));
}

void main()
{
    vec2 uv = v_texcoord0;
    float centerAO = texture2D(s_ssaoTex, uv).r;
    float centerDepth = getLinearDepth(uv);

    float aoAccum = centerAO;
    float weightAccum = 1.0;

    float texelX = 1.0 / u_ssaoParams.x;
    float texelY = 1.0 / u_ssaoParams.y;

    // Use a 5x5 box bilateral filter
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            if (x == 0 && y == 0)
                continue;

            vec2 offset = vec2(float(x) * texelX, float(y) * texelY);
            float sampleAO = texture2D(s_ssaoTex, uv + offset).r;
            float sampleDepth = getLinearDepth(uv + offset);

            // Bilateral weight based on depth difference to preserve edges
            float depthDiff = abs(sampleDepth - centerDepth);
            float weight = 1.0 / (1.0 + depthDiff * 25.0); // Adjust sensitivity

            aoAccum += sampleAO * weight;
            weightAccum += weight;
        }
    }

    float blurredAO = aoAccum / max(weightAccum, 0.0001);
    gl_FragColor = vec4(blurredAO, blurredAO, blurredAO, 1.0);
}
