#include "base/base_inc.h"
#include "platform/platform_inc.h"
#include "window_manager/window_manager_inc.h"
#include "render/render_inc.h"
#include "galaxy/galaxy_inc.h"
#include "ui/ui_inc.h"
#include "capymilk/capymilk_inc.h"

#include "base/base_inc.c"
#include "platform/platform_inc.c"
#include "window_manager/window_manager_inc.c"
#include "render/render_inc.c"
#include "galaxy/galaxy_inc.c"
#include "ui/ui_inc.c"
#include "capymilk/capymilk_inc.c"

int main(void) {
    plat_init();
    mem_arena *perm_arena = arena_alloc(MiB(4), KiB(64));
    mem_arena *glx_arena = arena_alloc(GiB(5), KiB(64));

    wm_window *window = wm_window_open(perm_arena, 1024, 1024, "Capymilk");

    r_backend_init();
    r_window_equip(window);

    cm_state state = cm_init(perm_arena, glx_arena, window);
    wm_window_set_draw_callback(window, cm_redraw, &state);

    while (!window->should_close) {
        wm_window_process_events(window);
        f32 dt = cm_tick(&state);
        cm_update(glx_arena, &state, dt);
        cm_draw(&state, dt);
    }

    return 0;
}
