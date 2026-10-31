#ifndef CAPYMILK_H
#define CAPYMILK_H

typedef struct {
    u32          seed;
    u64          start_us;
    u64          last_frame_us;
    wm_window   *window;
    glx_params   params;
    glx_vertex  *vertices;
    glx_renderer glx_render;
    ui_renderer  ui_render;
    r_timer      timer;
} cm_state;

internal cm_state cm_init(mem_arena *perm_arena, mem_arena *glx_arena, wm_window *window);
internal f32 cm_tick(cm_state *state);
internal void cm_update(mem_arena *arena, cm_state *state, f32 dt);
internal void cm_draw(cm_state *state, f32 dt);
internal void cm_redraw(void *ctx);

#endif
