$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_depthTex,     0);
SAMPLER2D(s_gbufferNormal,1); // View-space normal, packed [0,1]

uniform vec4 u_ssaoParams;       // x: screenWidth, y: screenHeight, z: radius, w: bias
uniform vec4 u_cameraProjParams; // x: tanHalfFovX, y: tanHalfFovY, z: nearPlane, w: farPlane
uniform vec4 u_ssaoParams2;      // x: intensity

float getLinearDepth(vec2 uv)
{
    float ndcDepth = texture2DLod(s_depthTex, uv, 0.0).r;
    float n = u_cameraProjParams.z;
    float f = u_cameraProjParams.w;
    return (n * f) / (f - ndcDepth * (f - n));
}

vec3 getViewPos(vec2 uv)
{
    float depth = getLinearDepth(uv);
    // UV Y is flipped vs NDC Y in DirectX/BGFX
    float ndcX =  (uv.x * 2.0 - 1.0);
    float ndcY = -(uv.y * 2.0 - 1.0);
    return vec3(ndcX * u_cameraProjParams.x * depth,
                ndcY * u_cameraProjParams.y * depth,
                -depth);
}

void main()
{
    vec2 uv = v_texcoord0;

    // Skip sky
    float rawD = texture2DLod(s_depthTex, uv, 0.0).r;
    if (rawD >= 0.9999)
    {
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    vec3 viewPos = getViewPos(uv);

    // Read exact GBuffer normal — no derivative artifacts on curved surfaces
    vec3 normal = texture2DLod(s_gbufferNormal, uv, 0.0).xyz * 2.0 - 1.0;
    normal = normalize(normal);

    // Interleaved gradient noise for rotation
    float noise = fract(52.9829189 * fract(dot(uv * u_ssaoParams.xy, vec2(0.06711056, 0.00583715))));
    float angle = noise * 6.2831853;
    float cosA = cos(angle);
    float sinA = sin(angle);

    // 16-sample hemisphere kernel
    vec3 kernel[16];
    kernel[0]  = vec3( 0.33,  0.00,  0.85);
    kernel[1]  = vec3(-0.17,  0.60,  0.50);
    kernel[2]  = vec3(-0.43, -0.10,  0.15);
    kernel[3]  = vec3( 0.05, -0.50,  0.75);
    kernel[4]  = vec3( 0.60,  0.15,  0.30);
    kernel[5]  = vec3(-0.15, -0.65,  0.45);
    kernel[6]  = vec3( 0.18,  0.30,  0.70);
    kernel[7]  = vec3(-0.35,  0.25,  0.20);
    kernel[8]  = vec3( 0.58, -0.30,  0.60);
    kernel[9]  = vec3(-0.75,  0.10,  0.35);
    kernel[10] = vec3( 0.00,  0.12,  0.92);
    kernel[11] = vec3( 0.15, -0.25,  0.80);
    kernel[12] = vec3(-0.20,  0.15,  0.40);
    kernel[13] = vec3( 0.35,  0.75,  0.10);
    kernel[14] = vec3(-0.50, -0.50,  0.25);
    kernel[15] = vec3( 0.05,  0.50,  0.60);

    float radius = u_ssaoParams.z;
    float bias   = u_ssaoParams.w;
    float occlusion = 0.0;

    // TBN from GBuffer normal + random tangent rotation
    vec3 rvec     = vec3(cosA, sinA, 0.0);
    vec3 tangent  = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    for (int i = 0; i < 16; ++i)
    {
        // Orient kernel sample to view space
        vec3 samplePos = mul(tbn, kernel[i]);
        samplePos = viewPos + samplePos * radius;

        // Project sample to screen UV (with Y flip)
        float invZ = 1.0 / (-samplePos.z);
        vec2 sampleUV;
        sampleUV.x =  (samplePos.x * invZ / u_cameraProjParams.x) * 0.5 + 0.5;
        sampleUV.y = -(samplePos.y * invZ / u_cameraProjParams.y) * 0.5 + 0.5;

        float sampleDepth = getLinearDepth(sampleUV);

        // Range check to prevent halos at depth discontinuities
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewPos.z - sampleDepth));

        occlusion += (sampleDepth <= -samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / 16.0) * u_ssaoParams2.x;
    occlusion = clamp(occlusion, 0.0, 1.0);

    gl_FragColor = vec4(occlusion, occlusion, occlusion, 1.0);
}
