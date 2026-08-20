#include <stdio.h>
#include <windows.h>
#include <gl/gl.h>

#include "opengl_defs.h"

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB  0x2011
#define WGL_PIXEL_TYPE_ARB     0x2013
#define WGL_TYPE_RGBA_ARB      0x202B
#define WGL_COLOR_BITS_ARB     0x2014
#define WGL_DEPTH_BITS_ARB     0x2022
#define WGL_STENCIL_BITS_ARB   0x2023

typedef HGLRC(_wglCreateContextAttribsARB_func)(HDC, HGLRC, const int*);
typedef BOOL(_wglChoosePixelFormatARB_func)(
    HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList,
    UINT nMaxFormats, int *piFormats, UINT *nNumFormats
);

global _wglCreateContextAttribsARB_func *wglCreateContextAttribsARB = NULL;
global _wglChoosePixelFormatARB_func    *wglChoosePixelFormatARB    = NULL;

global struct {
    i32   pixel_format;
    HDC   dc;
    HGLRC ctx;
} _w32gl_state;

internal void r_backend_init(void) {
    const wchar_t *dummy_class_name = L"DUMMY";
    HINSTANCE mod_handle = GetModuleHandleW(NULL);

    WNDCLASSW dummy_wnd_class = {
        .lpfnWndProc = DefWindowProcW,
        .hInstance = mod_handle,
        .lpszClassName = dummy_class_name
    };

    if (!RegisterClassW(&dummy_wnd_class)) {
        plat_fatal_err("failed to create dummy window class", 1);
    }

    HWND dummy_wnd = CreateWindowW(
        dummy_class_name, L"", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, mod_handle, NULL
    );
    if (!dummy_wnd) {
        plat_fatal_err("failed to create dummy window", 1);
    }

    HDC dummy_dc = GetDC(dummy_wnd);

    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
    };
    i32 pf = ChoosePixelFormat(dummy_dc, &pfd);
    DescribePixelFormat(dummy_dc, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    SetPixelFormat(dummy_dc, pf, &pfd);

    HGLRC dummy_ctx = wglCreateContext(dummy_dc);
    if (!dummy_ctx) {
        plat_fatal_err("failed to create dummy window ctx", 1);
    }

    wglMakeCurrent(dummy_dc, dummy_ctx);

    wglCreateContextAttribsARB = (_wglCreateContextAttribsARB_func*)
        wglGetProcAddress("wglCreateContextAttribsARB");
    wglChoosePixelFormatARB = (_wglChoosePixelFormatARB_func*)
        wglGetProcAddress("wglChoosePixelFormatARB");

    i32 pf_attribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,     32,
        WGL_DEPTH_BITS_ARB,     24,
        WGL_STENCIL_BITS_ARB,   8,
        0
    };
    u32 num_formats = 0;
    wglChoosePixelFormatARB(
        dummy_dc, pf_attribs, NULL, 1, &_w32gl_state.pixel_format, &num_formats
    );

    HMODULE gl_dll = GetModuleHandleW(L"opengl32.dll");
    #define X(ret, name, args)                                       \
        gl##name = (gl_##name##_func*)wglGetProcAddress("gl" #name); \
        if (!gl##name)                                               \
            gl##name = (gl_##name##_func*)GetProcAddress(gl_dll, "gl" #name);
    #   include "opengl_xlist.h"
    #undef X

    wglMakeCurrent(dummy_dc, NULL);
    wglDeleteContext(dummy_ctx);
    ReleaseDC(dummy_wnd, dummy_dc);
    DestroyWindow(dummy_wnd);
    UnregisterClassW(dummy_class_name, mod_handle);
}

internal void r_window_equip(wm_window *window) {
    HWND hwnd = window->plat_info->win_handle;
    _w32gl_state.dc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {0};
    DescribePixelFormat(_w32gl_state.dc, _w32gl_state.pixel_format, sizeof(pfd), &pfd);
    SetPixelFormat(_w32gl_state.dc, _w32gl_state.pixel_format, &pfd);

    i32 ctx_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        0
    };
    _w32gl_state.ctx = wglCreateContextAttribsARB(_w32gl_state.dc, NULL, ctx_attribs);
    if (!_w32gl_state.ctx) {
        plat_fatal_err("failed to create wgl ctx", 1);
    }

    wglMakeCurrent(_w32gl_state.dc, _w32gl_state.ctx);

    glEnable(GL_PROGRAM_POINT_SIZE); // for galaxy
}

internal void r_window_present(wm_window *window) {
    (void)window;
    SwapBuffers(_w32gl_state.dc);
}

internal r_shader r_shader_alloc(const char *vert_src, const char *frag_src) {
    r_shader shader = {0};
    GLuint vs = 0, fs = 0;
    i32 ok = 0;
    char info_log[512];

    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const char**)&vert_src, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(vs, 512, NULL, info_log);
        printf("vertex shader failed to compile: %s\n", info_log);
        goto cleanup;
    }

    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const char**)&frag_src, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(fs, 512, NULL, info_log);
        printf("fragment shader failed to compile: %s\n", info_log);
        goto cleanup;
    }

    shader.id = glCreateProgram();
    glAttachShader(shader.id, vs);
    glAttachShader(shader.id, fs);
    glLinkProgram(shader.id);
    glGetProgramiv(shader.id, GL_LINK_STATUS, &ok);
    if (!ok) {
        glGetProgramInfoLog(shader.id, 512, NULL, info_log);
        printf("shader program failed to link: %s\n", info_log);
        glDeleteProgram(shader.id);
        shader.id = 0;
    }

    cleanup:
    glDeleteShader(vs);
    glDeleteShader(fs);
    return shader;
}

internal void r_shader_use(r_shader shader) {
    glUseProgram(shader.id);
}

internal void r_uniform_f32(r_shader shader, const char *name, f32 v) {
    glProgramUniform1f(shader.id, glGetUniformLocation(shader.id, name), v);
}

internal void r_uniform_vec2(r_shader shader, const char *name, vec2 v) {
    glProgramUniform2f(shader.id, glGetUniformLocation(shader.id, name), v.x, v.y);
}

internal void r_uniform_vec3(r_shader shader, const char *name, vec3 v) {
    glProgramUniform3f(shader.id, glGetUniformLocation(shader.id, name), v.x, v.y, v.z);
}

internal r_buffer r_buffer_alloc(u64 size, const void *data, r_buffer_kind kind) {
    r_buffer buffer = {0};
    glCreateBuffers(1, &buffer.id);

    GLbitfield flags = (kind == r_buffer_dynamic) ? GL_DYNAMIC_STORAGE_BIT : 0;
    glNamedBufferStorage(buffer.id, size, data, flags);
    return buffer;
}

internal void r_buffer_release(r_buffer *buffer) {
    glDeleteBuffers(1, &buffer->id);
    buffer->id = 0;
}

internal void r_buffer_upload(r_buffer buffer, u64 size, const void *data) {
    glNamedBufferSubData(buffer.id, 0, size, data);
}

internal r_layout r_layout_alloc(const r_attrib *attribs, u32 count) {
    r_layout layout = {0};
    glCreateVertexArrays(1, &layout.id);

    u32 offset = 0;
    for (u32 i = 0; i < count; i++) {
        glVertexArrayAttribBinding(layout.id, i, 0);
        glVertexArrayAttribFormat(layout.id, i, attribs[i].count, GL_FLOAT, GL_FALSE, offset);
        glEnableVertexArrayAttrib(layout.id, i);
        offset += attribs[i].count * sizeof(f32);
    }

    return layout;
}

internal void r_layout_bind_buffer(r_layout layout, r_buffer buffer, u32 stride) {
    glVertexArrayVertexBuffer(layout.id, 0, buffer.id, 0, stride);
}

internal void r_layout_bind_index_buffer(r_layout layout, r_buffer buffer) {
    glVertexArrayElementBuffer(layout.id, buffer.id);
}

internal void r_blend_additive(void) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

internal void r_clear(vec3 color) {
    glClearColor(color.x, color.y, color.z, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

internal void r_viewport(u32 width, u32 height) {
    glViewport(0, 0, width, height);
}

internal void r_draw_points(r_layout layout, i32 n) {
    glBindVertexArray(layout.id);
    glDrawArrays(GL_POINTS, 0, n);
}

internal void r_draw_indexed(r_layout layout, i32 n) {
    glBindVertexArray(layout.id);
    glDrawElements(GL_TRIANGLES, n, GL_UNSIGNED_INT, 0);
}
