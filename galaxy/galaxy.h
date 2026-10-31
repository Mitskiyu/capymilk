#ifndef GALAXY_H
#define GALAXY_H

typedef struct {
    vec3 color;
    f32  radius;
    f32  theta;
    f32  ang_speed;
    f32  mag;
    f32  kind;
} glx_vertex;

typedef struct {
    i32 num_stars;
    i32 num_dust;
    i32 num_particles;
    f32 rad_core;
    f32 rad_galaxy;
    f32 time_scale;
    f32 ex_inner;
    f32 ex_outer;
    f32 angular_offset;
    f32 elapsed_years;
} glx_params;

#endif
