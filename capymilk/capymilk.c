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

    state.timer = r_timer_alloc();

    glx_ct_build();
    glx_init(&state.params, state.vertices, state.seed);
    glx_renderer_upload(&state.glx_render, state.vertices, state.params.num_particles);

    return state;
}

internal f32 cm_tick(cm_state *state) {
    u64 now_us = plat_time_usec();
    if (state->last_frame_us == 0) state->last_frame_us = now_us;
    f32 dt = (now_us - state->last_frame_us) / 1000000.0f;
    state->last_frame_us = now_us;

    glx_advance(&state->params, dt);
    return dt;
}

internal void cm_update(mem_arena *arena, cm_state *state, f32 dt) {
    glx_params *p = &state->params;
    b32 key_hit = false;

    if (WM_KEY_DOWN(state->window, 'Q')) { p->angular_offset += 0.0002f * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'A')) { p->angular_offset -= 0.0002f * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'W')) { p->ex_inner       += 0.1f    * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'S')) { p->ex_inner       -= 0.1f    * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'E')) { p->ex_outer       += 0.1f    * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'D')) { p->ex_outer       -= 0.1f    * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'R')) { p->time_scale     *= powf(2.0f, dt); key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'F')) { p->time_scale     /= powf(2.0f, dt); key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'U')) { p->rad_core       += 1000.0f * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'J')) { p->rad_core       -= 1000.0f * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'I')) { p->rad_galaxy     += 2000.0f * dt;   key_hit = true; }
    if (WM_KEY_DOWN(state->window, 'K')) { p->rad_galaxy     -= 2000.0f * dt;   key_hit = true; }

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

internal void cm_draw(cm_state *state, f32 dt) {
    local_persist u64 acc_frame_us = 0, acc_gpu_ns = 0;
    local_persist u32 frames = 0;

    f32 aspect_ratio = (f32)state->window->width / state->window->height;

    r_clear((vec3){0, 0, 0});
    r_viewport(state->window->width, state->window->height);

    glx_renderer_update(&state->glx_render, &state->params, aspect_ratio);

    r_timer_begin(state->timer);
    glx_renderer_draw(&state->glx_render, state->params.num_particles);
    r_timer_end();
    r_window_present(state->window);
    // ui_renderer_draw(&state->ui_render, 4.0f, 0.0f, 0.5f, 0.5f, (vec3){1,0,0});

    acc_gpu_ns += r_timer_result_ns(state->timer);
    acc_frame_us += (u64)(dt * 1000000.0f);
    if (++frames == 500) {
        printf("particles %d | frame %.2f ms | gpu %.2f ms\n",
            state->params.num_particles,
            acc_frame_us / 1000.0 / frames, acc_gpu_ns / 1e6 / frames);
        acc_frame_us = acc_gpu_ns = frames = 0;
    }
}

internal void cm_redraw(void *ctx) {
    cm_state *state = (cm_state *)ctx;
    cm_draw(state, cm_tick(state));
}
