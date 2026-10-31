#ifndef RENDER_H
#define RENDER_H

typedef struct { u32 id; } r_shader;
typedef struct { u32 id; } r_buffer;
typedef struct { u32 id; } r_layout;
typedef struct { u32 count; } r_attrib;
typedef struct { u32 id; } r_timer;

typedef enum {
    r_buffer_static,
    r_buffer_dynamic,
} r_buffer_kind;

internal void r_backend_init(void);
internal void r_window_equip(wm_window *window);
internal void r_window_present(wm_window *window);

internal r_shader r_shader_alloc(const char *vert_src, const char *frag_src);
internal void r_shader_use(r_shader shader);
internal void r_uniform_f32(r_shader shader, const char *name, f32 v);
internal void r_uniform_vec2(r_shader shader, const char *name, vec2 v);
internal void r_uniform_vec3(r_shader shader, const char *name, vec3 v);

internal r_buffer r_buffer_alloc(u64 size, const void *data, r_buffer_kind kind);
internal void r_buffer_release(r_buffer *buffer);
internal void r_buffer_upload(r_buffer buffer, u64 size, const void *data);

internal r_layout r_layout_alloc(const r_attrib *attribs, u32 count);
internal void r_layout_bind_buffer(r_layout layout, r_buffer buffer, u32 stride);
internal void r_layout_bind_index_buffer(r_layout layout, r_buffer buffer);

internal void r_blend_additive(void);
internal void r_clear(vec3 color);
internal void r_viewport(u32 width, u32 height);

internal void r_draw_points(r_layout layout, i32 n);
internal void r_draw_indexed(r_layout layout, i32 n);

internal r_timer r_timer_alloc(void);
internal void r_timer_begin(r_timer t);
internal void r_timer_end(void);
internal u64 r_timer_result_ns(r_timer t);

#endif
