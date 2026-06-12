$input a_position, a_normal, a_color0
$output v_worldNormal, v_color0, v_worldPosition

#include <bgfx_shader.sh>

void main()
{
    vec4 worldPosition = mul(u_model[0], vec4(a_position, 1.0));
    v_worldPosition = worldPosition.xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    v_color0 = a_color0;
    gl_Position = mul(u_viewProj, worldPosition);
}
