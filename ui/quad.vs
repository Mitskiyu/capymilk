#version 450

uniform vec2 u_pos;
uniform vec2 u_size;
uniform vec3 u_color;

layout (location = 0) in vec2 a_pos;

out vec3 v_color;

void main()
{
    vec2 pos = a_pos * u_size + u_pos;
    gl_Position = vec4(pos, 0.0, 1.0);
    v_color = u_color;
}
