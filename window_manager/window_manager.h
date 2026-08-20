#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#define WM_KEY_DOWN(win, key) (win->keys[key])
#define WM_KEY_UP(win, key) (!win->keys[key])
#define WM_KEY_JUST_DOWN(win, key) (win->keys[key] && !win->prev_keys[key])
#define WM_KEY_JUST_UP(win, key) (!win->keys[key] && win->prev_keys[key])

#define WM_MOUSE_DOWN(win, mb) (win->mouse_buttons[mb])
#define WM_MOUSE_UP(win, mb) (!win->mouse_buttons[mb])
#define WM_MOUSE_JUST_DOWN(win, mb) (win->mouse_buttons[mb] && !win->prev_mouse_buttons[mb])
#define WM_MOUSE_JUST_UP(win, mb) (!win->mouse_buttons[mb] && win->prev_mouse_buttons[mb])

typedef void (*wm_draw_fn)(void *ctx);

typedef enum {
    WM_MB_LEFT,
    WM_MB_RIGHT,
    WM_MB_MIDDLE,
    WM_MB_4,
    WM_MB_5,
     
    WM_MB_COUNT
} wm_mouse_button;

typedef struct {
    wm_draw_fn draw_callback;
    void      *draw_ctx;

    struct _wm_plat_info *plat_info;
    u32 width, height;

    vec2 mouse_pos;
    b8  mouse_buttons[WM_MB_COUNT];
    b8  prev_mouse_buttons[WM_MB_COUNT];

    b8  keys[256];
    b8  prev_keys[256];

    b32 should_close;
} wm_window;

internal wm_window *wm_window_open(mem_arena *arena, u32 w, u32 h, const char *title);
internal void wm_window_process_events(wm_window *window);
internal void wm_window_set_draw_callback(wm_window *window, wm_draw_fn fn, void *ctx);

#endif
