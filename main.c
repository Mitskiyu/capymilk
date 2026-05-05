#include <stdio.h>

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
    HDC hdc, HGLRC hshareContext, const int* attribList
);
typedef BOOL(wglChoosePixelFormatARB_func)(
    HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList,
    UINT nMaxFormats, int* piFormats, UINT* nNumFormats
);

static wglCreateContextAttribsARB_func* wglCreateContextAttribsARB = NULL;
static wglChoosePixelFormatARB_func* wglChoosePixelFormatARB = NULL;

#define DUMMY_CLASS_NAME L"dummy_class_name"
#define CLASS_NAME       L"class_name"

#define NUM_PTS 10000

LRESULT window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

typedef struct {
    f32 x, y, z;
} vertex;

typedef struct {
    b32 is_running;
    GLuint vao;
    GLuint shader_program;
    HDC dc;
} state;

const char* vert_shader_source =
    "#version 450\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "void main()\n"
    "{\n"
    "gl_Position = vec4(a_pos, 1.0);\n"
    "}\n\0";

const char* frag_shader_source =
    "#version 450\n"
    "out vec4 frag_color;\n"
    "void main()\n"
    "{\n"
    "frag_color = vec4(1.0f, 0.5f, 0.5f, 1.0f);\n"
    "}\n\0";

int main(void) {
    HINSTANCE mod_handle = GetModuleHandle(NULL);
    i32 pixel_format = 0;

    {
        WNDCLASSW dummy_wnd_class = {
            .lpfnWndProc = DefWindowProc,
            .hInstance = mod_handle,
            .lpszClassName = DUMMY_CLASS_NAME
        };
        if (!RegisterClassW(&dummy_wnd_class)) return 1;  // log

        HWND dummy_wnd = CreateWindowW(
            DUMMY_CLASS_NAME, L"", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, mod_handle, NULL
        );
        if (!dummy_wnd) return 1;

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
        if (!dummy_gl_context) return 1;  // log

        wglMakeCurrent(dummy_dc, dummy_gl_context);

        wglCreateContextAttribsARB = (wglCreateContextAttribsARB_func*)
            wglGetProcAddress("wglCreateContextAttribsARB");
        wglChoosePixelFormatARB = (wglChoosePixelFormatARB_func*)
            wglGetProcAddress("wglChoosePixelFormatARB");

        // clang-format off
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
        // clang-format on

        u32 num_formats = 0;
        wglChoosePixelFormatARB(
            dummy_dc, pf_attribs, NULL, 1, &pixel_format, &num_formats
        );

        HINSTANCE gl_dll = LoadLibraryA("opengl32.dll");
        // clang-format off
        // Load OpenGL funcs
        #define X(ret, name, args)                                       \
            gl##name = (gl_##name##_func*)wglGetProcAddress("gl" #name); \
            if (!gl##name)                                               \
                gl##name = (gl_##name##_func*)GetProcAddress(gl_dll, "gl" #name);
        #include "opengl_xlist.h"
        #undef X
        // clang-format on

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
    if (!RegisterClassW(&wnd_class)) return 1;  // log

    HWND window = CreateWindowExW(
        0, CLASS_NAME, L"Capymilk", WS_TILEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, mod_handle,
        NULL
    );
    if (!window) return 1;  // log

    HDC dc = GetDC(window);

    PIXELFORMATDESCRIPTOR pfd = {0};
    DescribePixelFormat(dc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
    SetPixelFormat(dc, pixel_format, &pfd);

    // clang-format off
    i32 ctx_attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        0
    };
    // clang-format on

    HGLRC gl_ctx = wglCreateContextAttribsARB(dc, NULL, ctx_attribs);
    if (!gl_ctx) return 1;  // log
    b32 ok = wglMakeCurrent(dc, gl_ctx);
    if (!ok) return 1;  // log

    u32 shader_program = 0;
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

        vertex vertices[NUM_PTS] = {0};
        for (size_t i = 0; i < NUM_PTS; i++) {
            float step = i * (360.0f / NUM_PTS);
            float radian = step * (PI / 180.0f);
            vertices[i].x = cosf(radian) * 0.5f;
            vertices[i].y = sinf(radian) * 0.5f;
            vertices[i].z = 0.0f;
        }

        glNamedBufferStorage(vbo, sizeof(vertices), vertices, 0);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex));
        glVertexArrayAttribBinding(vao, 0, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vao, 0);
    }

    state s = {
        .is_running = true,
        .shader_program = shader_program,
        .vao = vao,
        .dc = dc
    };
    SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)&s);

    glClearColor(0.5, 0.0, 0.5, 1.0);
    while (s.is_running) {
        MSG msg = {0};
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Draw
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(s.vao);
        glUseProgram(s.shader_program);
        glDrawArrays(GL_POINTS, 0, NUM_PTS);

        SwapBuffers(s.dc);
    }

    return 0;
}

static LRESULT window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    state* s = (state*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!s) return DefWindowProcW(hwnd, umsg, wparam, lparam);

    switch (umsg) {
        case WM_SIZE: {
            u32 width = LOWORD(lparam);
            u32 height = HIWORD(lparam);
            glViewport(0, 0, width, height);

            glClear(GL_COLOR_BUFFER_BIT);
            glBindVertexArray(s->vao);
            glUseProgram(s->shader_program);
            glDrawArrays(GL_POINTS, 0, NUM_PTS);

            SwapBuffers(s->dc);
        } break;
        case WM_CLOSE: {
            s->is_running = false;
        } break;
    }

    return DefWindowProcW(hwnd, umsg, wparam, lparam);
}
