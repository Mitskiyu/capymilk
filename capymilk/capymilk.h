#ifndef CAPYMILK_H
#define CAPYMILK_H

typedef struct {
    u32          seed;
    u64          start_us;
    wm_window   *window;
    glx_params   params;
    glx_vertex  *vertices;
    glx_renderer glx_render;
    ui_renderer  ui_render;
} cm_state;

internal cm_state cm_init(wm_window *window);
internal void cm_draw(void *ctx);
internal void cm_update(cm_state *state);

#endif
