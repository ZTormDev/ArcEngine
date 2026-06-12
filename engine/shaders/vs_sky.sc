$input a_position
$output v_skyDirection

#include <bgfx_shader.sh>

void main()
{
    v_skyDirection = a_position;
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
