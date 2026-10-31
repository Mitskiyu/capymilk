#include <stdio.h>

internal glx_renderer glx_renderer_create(mem_arena *arena, i32 n) {
    glx_renderer render = {0};

    r_shader shader = {0};
    {
        char *vert_source = plat_file_read(arena, "galaxy/points.vs");
        if (vert_source == NULL) {
            printf("failed to load points.vs\n");
            return render;
        }

        char *frag_source = plat_file_read(arena, "galaxy/points.fs");
        if (frag_source == NULL) {
            printf("failed to load points.fs\n");
            return render;
        }

        shader = r_shader_alloc(vert_source, frag_source);
        if (shader.id == 0) return render;
    }

    r_attrib attribs[6] = { {3}, {1}, {1}, {1}, {1}, {1} };

    render.shader = shader;
    render.vao = r_layout_alloc(attribs, 6);
    glx_renderer_resize(&render, n);

    r_blend_additive();

    return render;
}

internal void glx_renderer_resize(glx_renderer *render, i32 n) {
    if (render->vbo.id) r_buffer_release(&render->vbo);

    render->vbo = r_buffer_alloc(sizeof(glx_vertex) * n, NULL, r_buffer_dynamic);
    r_layout_bind_buffer(render->vao, render->vbo, sizeof(glx_vertex));
}

internal void glx_renderer_draw(const glx_renderer *render, i32 n) {
    r_shader_use(render->shader);
    r_draw_points(render->vao, n);
}

internal void glx_renderer_upload(const glx_renderer *render, const glx_vertex *vertices, i32 n) {
    r_buffer_upload(render->vbo, sizeof(glx_vertex) * n, vertices);
}

internal void glx_renderer_update(const glx_renderer *render, const glx_params *params, f32 aspect_ratio) {
    r_uniform_f32(render->shader, "u_aspect_ratio",   aspect_ratio);
    r_uniform_f32(render->shader, "u_elapsed_years",  params->elapsed_years);
    r_uniform_f32(render->shader, "u_ex_inner",       params->ex_inner);
    r_uniform_f32(render->shader, "u_ex_outer",       params->ex_outer);
    r_uniform_f32(render->shader, "u_rad_core",       params->rad_core);
    r_uniform_f32(render->shader, "u_rad_galaxy",     params->rad_galaxy);
    r_uniform_f32(render->shader, "u_angular_offset", params->angular_offset);
}
