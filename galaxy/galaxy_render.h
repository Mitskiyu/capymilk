#ifndef GALAXY_RENDER_H
#define GALAXY_RENDER_H

typedef struct {
    r_layout vao;
    r_buffer vbo;
    r_shader shader;
} glx_renderer;

internal glx_renderer glx_renderer_create(mem_arena *arena, i32 n);
internal void glx_renderer_resize(glx_renderer *render, i32 n);
internal void glx_renderer_draw(const glx_renderer *render, i32 n);
internal void glx_renderer_upload(const glx_renderer *render, const glx_vertex *vertices, i32 n);
internal void glx_renderer_update(const glx_renderer *render, const glx_params *params, f32 elapsed_years, f32 aspect_ratio);

#endif
