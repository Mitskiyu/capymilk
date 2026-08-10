#version 450

uniform float u_elapsed_years;
uniform float u_ex_inner;
uniform float u_ex_outer;
uniform float u_rad_core;
uniform float u_rad_galaxy;
uniform float u_angular_offset;

layout (location = 0) in vec3  a_color;
layout (location = 1) in float a_radius;
layout (location = 2) in float a_theta;
layout (location = 3) in float a_ang_speed;
layout (location = 4) in float a_mag;
layout (location = 5) in float a_type;

out vec3 v_color;
flat out float v_type;

float eccentricity(float rad_core, float rad_galaxy, float ex_inner, float ex_outer, float radius)
{
    float rad_farfield = rad_galaxy * 2;
    if (radius < rad_core) {
        float t = radius / rad_core;
        return 1.0 + t * (ex_inner - 1.0);
    } else if (radius < rad_galaxy) {
        float t = (radius - rad_core) / (rad_galaxy - rad_core);
        return ex_inner + t * (ex_outer - ex_inner);
    } else if (radius < rad_farfield) {
        float t = (radius - rad_galaxy) / (rad_farfield - rad_galaxy);
        return ex_outer + t * (1.0 - ex_outer);
    } else return 1.0;
}

void main()
{
    float scale = 0.5 / u_rad_galaxy;

    float theta = a_theta + (a_ang_speed * u_elapsed_years);
    float tilt = a_radius * u_angular_offset;
    float x = cos(theta) * a_radius;
    float y = sin(theta) * a_radius * eccentricity(u_rad_core, u_rad_galaxy, u_ex_inner, u_ex_outer, a_radius);
    float cos_r = cos(tilt);
    float sin_r = sin(tilt);
    vec3 pos = vec3(0.0);

    pos.x = (x * cos_r - y * sin_r) * scale;
    pos.y = (x * sin_r + y * cos_r) * scale;

    gl_Position = vec4(pos, 1.0);
    if (a_type == 0.0) 
        gl_PointSize = 4.0 * a_mag;
    else if (a_type == 1.0)
        gl_PointSize = 5.0 * 70.0 * a_mag;
    else
        gl_PointSize = 70.0 * a_mag;

    v_color = a_color * a_mag;
    v_type = a_type;
}
