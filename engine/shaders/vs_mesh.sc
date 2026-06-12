$input a_position, a_normal, a_color0, a_texcoord0, a_tangent
$output v_worldNormal, v_color0, v_worldPosition, v_texcoord0, v_worldTangent, v_currClipPos, v_prevClipPos

#include <bgfx_shader.sh>

uniform mat4 u_prevModel;
uniform mat4 u_prevViewProj;
uniform mat4 u_currentViewProj;

void main()
{
    vec4 worldPosition = mul(u_model[0], vec4(a_position, 1.0));
    v_worldPosition = worldPosition.xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    v_worldTangent = vec4(normalize(mul(u_model[0], vec4(a_tangent.xyz, 0.0)).xyz), a_tangent.w);
    v_texcoord0 = a_texcoord0;
    v_color0 = a_color0;

    v_currClipPos = mul(u_currentViewProj, worldPosition);
    vec4 prevWorldPos = mul(u_prevModel, vec4(a_position, 1.0));
    v_prevClipPos = mul(u_prevViewProj, prevWorldPos);

    gl_Position = mul(u_viewProj, worldPosition);
}

