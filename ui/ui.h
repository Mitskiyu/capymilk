#ifndef UI_H
#define UI_H

typedef struct {
    vec2 pos;
    vec3 color;
} ui_vertex;

typedef struct {
    r_layout vao;
    r_buffer vbo;
    r_buffer ebo;
    r_shader shader;
} ui_renderer;

internal ui_renderer ui_renderer_create(mem_arena *arena);
internal void ui_renderer_draw(const ui_renderer *render, f32 x, f32 y, f32 w, f32 h, vec3 color);

#endif
