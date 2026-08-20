#include <stdio.h>
#include <stdlib.h>

internal ui_renderer ui_renderer_create(mem_arena *arena) {
    ui_renderer render = {0};

    r_shader shader = {0};
    {
        char *vert_source = plat_file_read(arena, "ui/quad.vs");
        if (vert_source == NULL) {
            printf("failed to load quad.vs\n");
            return render;
        }

        char *frag_source = plat_file_read(arena, "ui/quad.fs");
        if (frag_source == NULL) {
            printf("failed to load quad.fs\n");
            return render;
        }

        shader = r_shader_alloc(vert_source, frag_source);
        if (shader.id == 0) return render;
    }

    ui_vertex quad[4] = {
        { .pos = { -1.0f,  1.0f } }, // TL
        { .pos = {  1.0f,  1.0f } }, // TR
        { .pos = { -1.0f, -1.0f } }, // BL
        { .pos = {  1.0f, -1.0f } }, // BR
    };

    u32 indices[6] = {
        0, 1, 2,
        2, 1, 3
    };

    r_attrib attribs[2] = { {2}, {3} };

    render.shader = shader;
    render.vbo = r_buffer_alloc(sizeof(quad), quad, r_buffer_static);
    render.ebo = r_buffer_alloc(sizeof(indices), indices, r_buffer_static);
    render.vao = r_layout_alloc(attribs, 2);

    r_layout_bind_buffer(render.vao, render.vbo, sizeof(ui_vertex));
    r_layout_bind_index_buffer(render.vao, render.ebo);

    return render;
}

internal void ui_renderer_draw(const ui_renderer *render, f32 x, f32 y, f32 w, f32 h, vec3 color) {
    r_shader_use(render->shader);

    r_uniform_vec2(render->shader, "u_pos",   (vec2){ x, y });
    r_uniform_vec2(render->shader, "u_size",  (vec2){ w, h });
    r_uniform_vec3(render->shader, "u_color", color);

    r_draw_indexed(render->vao, 6);
}
