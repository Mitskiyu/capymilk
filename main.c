#include <stdio.h>
#include <stdlib.h>

#define UNICODE
#include <windows.h>
#include <gl/gl.h>

#include "base.h"
#include "opengl.h"

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

typedef HGLRC(wglCreateContextAttribsARB_func)(
    HDC hdc, HGLRC hshareContext, const int *attribList
);
typedef BOOL(wglChoosePixelFormatARB_func)(
    HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList,
    UINT nMaxFormats, int *piFormats, UINT *nNumFormats
);

global wglCreateContextAttribsARB_func *wglCreateContextAttribsARB = NULL;
global wglChoosePixelFormatARB_func    *wglChoosePixelFormatARB    = NULL;

#define DUMMY_CLASS_NAME L"dummy_class_name"
#define CLASS_NAME       L"class_name"

#define NUM_PTS      200
#define NUM_CIRCLES  25
#define RAD_GALAXY   13000.0f
#define RAD_CORE     3000.0f
#define RAD_FARFIELD (RAD_GALAXY * 2.0f)

#define I0_DISK   1.0f
#define I0_BULGE  1.0f
#define K         0.02f
#define A         (RAD_GALAXY / 3.0f)

typedef struct {
    f32 x, y, z;
} vertex_t;

typedef struct {
    HINSTANCE mod_handle;
    HWND      window;
    HDC       device_ctx;
    HGLRC     gl_ctx;
} platform_t;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint shader_program;
} renderer_t;

typedef struct {
    f32 angular_offset;
    f32 ex_inner;
    f32 ex_outer;
} galaxy_params_t;

typedef struct {
    b32        is_running;
    platform_t *platform;
    renderer_t *renderer;
    galaxy_params_t *galaxy_params;
} app_t;

platform_t platform_create(void);
renderer_t renderer_create(void);
void renderer_draw(renderer_t *renderer);
void renderer_upload(renderer_t *renderer, vertex_t *vertices);
f32 galaxy_eccentricity(galaxy_params_t *params, f32 radius);
f32 galaxy_brightness(f32 radius);
vertex_t *galaxy_generate(galaxy_params_t *params);
LRESULT window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

global const char* vert_shader_source =
    "#version 450\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "void main()\n"
    "{\n"
    "gl_Position = vec4(a_pos, 1.0);\n"
    "}\n\0";

global const char* frag_shader_source =
    "#version 450\n"
    "out vec4 frag_color;\n"
    "void main()\n"
    "{\n"
    "frag_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
    "}\n\0";

int main(void) {
    platform_t platform = platform_create();
    if (!platform.window) return 1;

    renderer_t renderer = renderer_create();
    galaxy_params_t galaxy_params = {-0.14f, 0.8f, 1.0f};
    app_t app = {
        .is_running = true,
        .platform = &platform,
        .renderer = &renderer,
        .galaxy_params = &galaxy_params,
    };

    SetWindowLongPtrW(platform.window, GWLP_USERDATA, (LONG_PTR)&app);

    vertex_t *vertices = galaxy_generate(&galaxy_params);
    renderer_upload(&renderer, vertices);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0);
    while (app.is_running) {
        MSG msg = {0};
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Draw
        renderer_draw(&renderer);

        SwapBuffers(platform.device_ctx);
    }

    return 0;
}

internal platform_t platform_create(void) {
    platform_t platform = {0};

    HINSTANCE mod_handle = GetModuleHandle(NULL);
    i32 pixel_format = 0;
    {
        WNDCLASSW dummy_wnd_class = {
            .lpfnWndProc = DefWindowProc,
            .hInstance = mod_handle,
            .lpszClassName = DUMMY_CLASS_NAME
        };
        if (!RegisterClassW(&dummy_wnd_class)) return platform;  // log

        HWND dummy_wnd = CreateWindowW(
            DUMMY_CLASS_NAME, L"", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, mod_handle, NULL
        );
        if (!dummy_wnd) return platform;

        HDC dummy_dc = GetDC(dummy_wnd);

        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags =
                PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 32,
        };
        i32 pf = ChoosePixelFormat(dummy_dc, &pfd);
        DescribePixelFormat(dummy_dc, pf, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        SetPixelFormat(dummy_dc, pf, &pfd);

        HGLRC dummy_gl_context = wglCreateContext(dummy_dc);
        if (!dummy_gl_context) return platform;  // log

        wglMakeCurrent(dummy_dc, dummy_gl_context);

        wglCreateContextAttribsARB = (wglCreateContextAttribsARB_func*)
            wglGetProcAddress("wglCreateContextAttribsARB");
        wglChoosePixelFormatARB = (wglChoosePixelFormatARB_func*)
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
            dummy_dc, pf_attribs, NULL, 1, &pixel_format, &num_formats
        );

        HINSTANCE gl_dll = LoadLibraryA("opengl32.dll");
        // Load OpenGL funcs
        #define X(ret, name, args)                                       \
            gl##name = (gl_##name##_func*)wglGetProcAddress("gl" #name); \
            if (!gl##name)                                               \
                gl##name = (gl_##name##_func*)GetProcAddress(gl_dll, "gl" #name);
        #include "opengl_xlist.h"
        #undef X

        FreeLibrary(gl_dll);
        wglMakeCurrent(dummy_dc, NULL);
        wglDeleteContext(dummy_gl_context);
        ReleaseDC(dummy_wnd, dummy_dc);
        DestroyWindow(dummy_wnd);
        UnregisterClassW(DUMMY_CLASS_NAME, mod_handle);
    }

    WNDCLASSW wnd_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = window_proc,
        .hInstance = mod_handle,
        .lpszClassName = CLASS_NAME,
        .hCursor = LoadCursorW(0, IDC_ARROW)
    };
    if (!RegisterClassW(&wnd_class)) return platform;  // log

    HWND window = CreateWindowExW(
        0, CLASS_NAME, L"Capymilk", WS_TILEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, 1024, 1024, NULL, NULL, mod_handle,
        NULL
    );
    if (!window) return platform;  // log

    HDC dc = GetDC(window);

    PIXELFORMATDESCRIPTOR pfd = {0};
    DescribePixelFormat(dc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    SetPixelFormat(dc, pixel_format, &pfd);

    i32 ctx_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        0
    };

    HGLRC gl_ctx = wglCreateContextAttribsARB(dc, NULL, ctx_attribs);
    if (!gl_ctx) return platform;  // log
    b32 ok = wglMakeCurrent(dc, gl_ctx);
    if (!ok) return platform;  // log
    
    platform.mod_handle = mod_handle;
    platform.window = window;
    platform.device_ctx = dc;
    platform.gl_ctx = gl_ctx;

    return platform;
}

internal renderer_t renderer_create(void) {
    renderer_t renderer = {0};

    GLuint shader_program = 0;
    {
        i32 ok = 0;
        char info_log[512];

        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &vert_shader_source, NULL);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
            printf("vertex shader failed to compile: %s\n", info_log);
        }

        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &frag_shader_source, NULL);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
            printf("fragment shader failed to compile: %s\n", info_log);
        }

        shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
        glGetProgramiv(shader_program, GL_LINK_STATUS, &ok);
        if (!ok) {
            glGetProgramInfoLog(shader_program, 512, NULL, info_log);
            printf("shader program failed to link: %s\n", info_log);
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    GLuint vbo = 0, vao = 0;
    {
        glCreateBuffers(1, &vbo);
        glCreateVertexArrays(1, &vao);

        glNamedBufferStorage(vbo, sizeof(vertex_t) * NUM_CIRCLES * NUM_PTS, NULL, GL_DYNAMIC_STORAGE_BIT);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex_t));
        glVertexArrayAttribBinding(vao, 0, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vao, 0);
    }

    renderer.shader_program = shader_program;
    renderer.vao = vao;
    renderer.vbo = vbo;

    return renderer;
}

internal void renderer_draw(renderer_t *renderer) {
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(renderer->vao);
    glUseProgram(renderer->shader_program);
    glDrawArrays(GL_POINTS, 0, NUM_CIRCLES * NUM_PTS);
}

internal void renderer_upload(renderer_t *renderer, vertex_t *vertices) {
    glNamedBufferSubData(renderer->vbo, 0, sizeof(vertex_t) * NUM_CIRCLES * NUM_PTS, vertices);
}

internal f32 galaxy_eccentricity(galaxy_params_t *params, f32 radius) {
    if (radius < RAD_CORE) {
        f32 t = radius / RAD_CORE;
        return 1.0f + t * (params->ex_inner - 1.0f);
    } else if (radius < RAD_GALAXY) {
        f32 t = (radius - RAD_CORE) / (RAD_GALAXY - RAD_CORE);
        return params->ex_inner + t * (params->ex_outer - params->ex_inner);
    } else if (radius < RAD_FARFIELD) {
        f32 t = (radius - RAD_GALAXY) / (RAD_FARFIELD - RAD_GALAXY);
        return params->ex_outer + t * (1.0f - params->ex_outer);
    } else {
        return 1.0f;
    }
}

internal f32 galaxy_brightness(f32 radius) {
    f32 bulge = I0_BULGE * expf(-K * powf(radius, 0.25f));
    f32 disk  = I0_DISK * expf(-radius / A);
    return bulge + disk;
}

internal vertex_t *galaxy_generate(galaxy_params_t *params) {
    local_persist vertex_t vertices[NUM_CIRCLES * NUM_PTS];
    f32 scale = 0.5f / RAD_GALAXY;

    for (i32 circle = 0; circle < NUM_CIRCLES; circle++) {
        f32 radius = ((f32)(circle + 1) / NUM_CIRCLES) * RAD_GALAXY;
        f32 rotation = circle * params->angular_offset;
        f32 cos_r = cosf(rotation);
        f32 sin_r = sinf(rotation);

        for (i32 point = 0; point < NUM_PTS; point++) {
            f32 theta = (rand() / (f32)RAND_MAX) * 2.0f * PI;
            i32 idx = circle * NUM_PTS + point;
            f32 x = cosf(theta) * radius;
            f32 y = sinf(theta) * radius * galaxy_eccentricity(params, radius);
            vertices[idx].x = (x * cos_r - y * sin_r) * scale;
            vertices[idx].y = (x * sin_r + y * cos_r) * scale;
        }
    }

    return vertices;
}

internal LRESULT window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    app_t *app = (app_t*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!app) return DefWindowProcW(hwnd, umsg, wparam, lparam);

    switch (umsg) {
        case WM_KEYDOWN: {
            switch (wparam) {
                case 'Q': app->galaxy_params->angular_offset += 0.005f; break;
                case 'A': app->galaxy_params->angular_offset -= 0.005f; break;
                case 'W': app->galaxy_params->ex_inner       += 0.02f;  break;
                case 'S': app->galaxy_params->ex_inner       -= 0.02f;  break;
                case 'E': app->galaxy_params->ex_outer       += 0.02f;  break;
                case 'D': app->galaxy_params->ex_outer       -= 0.02f;  break;
                default: return DefWindowProcW(hwnd, umsg, wparam, lparam);
            }
            renderer_upload(app->renderer, galaxy_generate(app->galaxy_params));
            printf("offset: %f  ex_inner: %f ex_outer: %f\n", app->galaxy_params->angular_offset,
                                                                      app->galaxy_params->ex_inner, 
                                                                      app->galaxy_params->ex_outer);
        } break;
        case WM_SIZE: {
            u32 width = LOWORD(lparam);
            u32 height = HIWORD(lparam);
            glViewport(0, 0, width, height);

            renderer_draw(app->renderer);

            SwapBuffers(app->platform->device_ctx);
        } break;
        case WM_CLOSE: {
            app->is_running = false;
        } break;
    }

    return DefWindowProcW(hwnd, umsg, wparam, lparam);
}
