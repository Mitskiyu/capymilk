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

    wm_window *window = wm_window_open(1024, 1024, "Capymilk");
    if (!window) return 1;

    r_backend_init();
    r_window_equip(window);

    cm_state state = cm_init(window);
    wm_window_set_draw_callback(window, cm_draw, &state);

    while (!window->should_close) {
        wm_window_process_events(window);
        cm_update(&state);
        cm_draw(&state);
    }

    return 0;
}
