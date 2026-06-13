$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_depthTex, 0); // Raw full-resolution depth

uniform vec4 u_cameraProjParams; // x: tanHalfFovX, y: tanHalfFovY, z: nearPlane, w: farPlane

void main()
{
    float rawDepth = texture2D(s_depthTex, v_texcoord0).r;
    float n = u_cameraProjParams.z;
    float f = u_cameraProjParams.w;
    
    // Linearize depth
    float linearDepth = (n * f) / (f - rawDepth * (f - n));
    
    gl_FragColor = vec4(linearDepth, linearDepth, linearDepth, 1.0);
}
