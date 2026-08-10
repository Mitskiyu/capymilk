#version 450

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_color;
layout (location = 2) in float a_mag;
layout (location = 3) in float a_type;

out vec3 v_color;
flat out float v_type;

void main()
{
    gl_Position = vec4(a_pos, 1.0);
    if (a_type == 0.0) 
        gl_PointSize = 4.0 * a_mag;
    else if (a_type == 1.0)
        gl_PointSize = 5.0 * 70.0 * a_mag;
    else
        gl_PointSize = 70.0 * a_mag;

    v_color = a_color * a_mag;
    v_type = a_type;
}
