#include <stdio.h>
#include <time.h>

internal cm_state cm_init(mem_arena *perm_arena, mem_arena *glx_arena, wm_window *window) {
    cm_state state = {0};

    state.seed     = (u32)time(NULL);
    state.start_us = plat_time_usec();
    state.window   = window;
    state.params   = (glx_params){
        .num_stars      = 60000,
        .num_dust       = 60000,
        .rad_core       = 6000.0f,
        .rad_galaxy     = 15000.0f,
        .time_scale     = 10000000.0f,
        .ex_inner       = 0.8f,
        .ex_outer       = 1.0f,
        .angular_offset = -0.0004f,
    };

    i32 capacity = state.params.num_stars + state.params.num_dust * 2;

    state.vertices = PUSH_ARRAY_NZ(glx_arena, glx_vertex, capacity);

    state.ui_render  = ui_renderer_create(perm_arena);
    state.glx_render = glx_renderer_create(perm_arena, capacity);

    glx_ct_build();
    glx_init(&state.params, state.vertices, state.seed);
    glx_renderer_upload(&state.glx_render, state.vertices, state.params.num_particles);

    return state;
}

internal void cm_draw(void *ctx) {
    cm_state *state = (cm_state*)ctx;
    local_persist u64 last_frame_us = 0;
    local_persist f32 elapsed_years = 0.0f;

    u64 now_us = plat_time_usec();
    if (last_frame_us == 0) last_frame_us = now_us;

    f32 dt_sec = (now_us - last_frame_us) / 1000000.0f;
    elapsed_years += dt_sec * state->params.time_scale;
    last_frame_us = now_us;

    f32 aspect_ratio = (f32)state->window->width / state->window->height;

    r_clear((vec3){0, 0, 0});
    r_viewport(state->window->width, state->window->height);

    glx_renderer_update(&state->glx_render, &state->params, elapsed_years, aspect_ratio);
    glx_renderer_draw(&state->glx_render, state->params.num_particles);
    // ui_renderer_draw(&state->ui_render, 4.0f, 0.0f, 0.5f, 0.5f, (vec3){1,0,0});

    r_window_present(state->window);
}

internal void cm_update(mem_arena *arena, cm_state *state) {
    b32 key_hit = false;
    if (WM_KEY_DOWN(state->window, 'Q')) { state->params.angular_offset += 0.00005f;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'A')) { state->params.angular_offset -= 0.00005f;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'W')) { state->params.ex_inner       += 0.02f;      key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'S')) { state->params.ex_inner       -= 0.02f;      key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'E')) { state->params.ex_outer       += 0.02f;      key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'D')) { state->params.ex_outer       -= 0.02f;      key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'R')) { state->params.time_scale     += 1000000.0f; key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'F')) { state->params.time_scale     -= 1000000.0f; key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'U')) { state->params.rad_core       += 100.0f;     key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'J')) { state->params.rad_core       -= 100.0f;     key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'I')) { state->params.rad_galaxy     += 100.0f;     key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'K')) { state->params.rad_galaxy     -= 100.0f;     key_hit = true; }

    b32 should_update = false;
    if (WM_KEY_DOWN(state->window, 'T')) {
        state->params.num_stars += 1000;
        key_hit = true;
        should_update = true;
    }
    if (WM_KEY_DOWN(state->window, 'G')) {
        state->params.num_stars -= 1000;
        if (state->params.num_stars < 0)
            state->params.num_stars = 0;
        key_hit = true;
        should_update = true;
    }
    if (WM_KEY_DOWN(state->window, 'Y')) {
        state->params.num_dust += 1000;
        key_hit = true;
        should_update = true;
    }
    if (WM_KEY_DOWN(state->window, 'H')) {
        state->params.num_dust -= 1000;
        if (state->params.num_dust < 0)
            state->params.num_dust = 0;
        key_hit = true;
        should_update = true;
    }

    if (should_update) {
        i32 capacity = state->params.num_stars + state->params.num_dust * 2;

        arena_clear(arena);
        state->vertices = PUSH_ARRAY_NZ(arena, glx_vertex, capacity);
        glx_renderer_resize(&state->glx_render, capacity);
        glx_init(&state->params, state->vertices, state->seed);
        glx_renderer_upload(&state->glx_render, state->vertices, state->params.num_particles);
    }

    if (key_hit) {
        printf("offset: %f ex_inner: %f ex_outer: %f time_scale %.0f num_stars %d num_dust %d rad_core %.0f rad_galaxy %.0f\n",
            state->params.angular_offset, state->params.ex_inner,  state->params.ex_outer,
            state->params.time_scale,     state->params.num_stars, state->params.num_dust,
            state->params.rad_core,       state->params.rad_galaxy);
    }
}
