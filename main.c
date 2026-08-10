#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define UNICODE
#include <windows.h>
#include <gl/gl.h>

#include "base_core.h"
#include "base_math.h"
#include "opengl.h"

#include "base_math.c"

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

#define I0_BULGE   1.0f
#define K          0.02f

#define CDF_STEPS  1000
#define CDF_POINTS (CDF_STEPS / 2)

#define PLANCK_C1 3.74183e-16f
#define PLANCK_C2 1.4388e-2f

#define COLOR_TABLE_SIZE 1000
#define TEMP_MIN         1000.0f
#define TEMP_MAX         12000.0f

#define G             6.672e-11f
#define PC_TO_KM      3.08567758129e13f
#define SEC_PER_YEAR  (365.25f * 86400.0f)

typedef struct {
    vec3 color;
    f32  radius;
    f32  theta;
    f32  ang_speed;
    f32  mag;
    f32  type;
} vertex;

typedef struct {
    HINSTANCE mod_handle;
    HWND      window;
    HDC       device_ctx;
    HGLRC     gl_ctx;
} platform;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint shader_program;
} renderer;

typedef struct {
    i32 num_stars;
    i32 num_dust;
    f32 rad_core;
    f32 rad_galaxy;
    f32 time_scale;
    f32 ex_inner;
    f32 ex_outer;
    f32 angular_offset;
} galaxy_params;

typedef struct {
    b32           is_running;
    vertex        *vertices;
    platform      *platform;
    renderer      *renderer;
    galaxy_params *galaxy_params;
} app;

internal platform platform_create(void);
internal char *platform_read_file(const char *path);
internal LRESULT platform_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
internal renderer renderer_create(i32 n);
internal void renderer_resize(renderer *render, i32 n);
internal void renderer_draw(const renderer *render, i32 n);
internal void renderer_upload(renderer *render, vertex *vertices, i32 n);
internal void renderer_update(renderer *render, galaxy_params *params, f32 elapsed_years);
internal f32 galaxy_ex(const galaxy_params *params, f32 radius);
internal f32 galaxy_brightness(f32 rad_core, f32 rad_galaxy, f32 radius);
internal void galaxy_cdf_build(f32 *radii, f32 *cumulative, f32 *inverse, f32 rad_core, f32 rad_galaxy);
internal f32 galaxy_cdf_sample(const f32 *inverse, f32 p);
internal f32 galaxy_bb_spectrum(i32 wavelength, f32 temp);
internal vec3 galaxy_spectrum_to_xyz(f32 temp);
internal vec3 galaxy_xyz_to_rgb(vec3 xyz);
internal vec3 galaxy_bb_color(f32 temp);
internal void galaxy_ct_build(void);
internal vec3 galaxy_ct_lookup(f32 temp);
internal f32 galaxy_mass_disc(f32 rad_core, f32 r);
internal f32 galaxy_mass_halo(f32 rad_galaxy, f32 r);
internal f32 galaxy_velocity_kms(f32 rad_core, f32 rad_galaxy, f32 r);
internal f32 galaxy_orbital_velocity(f32 rad_core, f32 rad_galaxy, f32 r);
internal void galaxy_init(galaxy_params *params, vertex *vertices);

global vec3 color_table[COLOR_TABLE_SIZE];

int main(void) {
    srand((u32)time(NULL));

    platform plat = platform_create();
    if (!plat.window) return 1;

    galaxy_params params = {
        .num_stars = 60000,
        .num_dust = 60000,
        .rad_core = 6000.0f,
        .rad_galaxy = 15000.0f,
        .time_scale = 10000000.0f,
        .ex_inner = 0.8f,
        .ex_outer = 1.0f,
        .angular_offset = -0.0004f,
    };

    i32 num_particles = params.num_stars + params.num_dust * 2;

    vertex *vertices = (vertex *)malloc(num_particles * sizeof(vertex));
    if (!vertices) return 1;

    renderer render = renderer_create(num_particles);
    app state = {
        .is_running = true,
        .vertices = vertices,
        .platform = &plat,
        .renderer = &render,
        .galaxy_params = &params,
    };

    galaxy_ct_build();
    galaxy_init(&params, vertices);
    renderer_upload(&render, vertices, num_particles);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0);

    SetWindowLongPtrW(plat.window, GWLP_USERDATA, (LONG_PTR)&state);
    LARGE_INTEGER start_time, end_time, tps;
    QueryPerformanceFrequency(&tps);
    QueryPerformanceCounter(&start_time);
    while (state.is_running) {
        MSG msg = {0};
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        QueryPerformanceCounter(&end_time);
        u64 elapsed_us = ((end_time.QuadPart - start_time.QuadPart) * 1000000) / tps.QuadPart;
        f32 elapsed_years = (elapsed_us / 1000000.0f) * params.time_scale;

        num_particles = state.galaxy_params->num_stars + state.galaxy_params->num_dust * 2;
        renderer_update(&render, &params, elapsed_years);
        renderer_upload(&render, state.vertices, num_particles);

        // Draw
        renderer_draw(&render, num_particles);

        SwapBuffers(plat.device_ctx);
    }

    return 0;
}

internal platform platform_create(void) {
    platform plat = {0};

    HINSTANCE mod_handle = GetModuleHandle(NULL);
    i32 pixel_format = 0;
    {
        WNDCLASSW dummy_wnd_class = {
            .lpfnWndProc = DefWindowProc,
            .hInstance = mod_handle,
            .lpszClassName = DUMMY_CLASS_NAME
        };
        if (!RegisterClassW(&dummy_wnd_class)) return plat;  // log

        HWND dummy_wnd = CreateWindowW(
            DUMMY_CLASS_NAME, L"", WS_TILEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, mod_handle, NULL
        );
        if (!dummy_wnd) return plat;

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
        if (!dummy_gl_context) return plat;  // log

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
        .lpfnWndProc = platform_window_proc,
        .hInstance = mod_handle,
        .lpszClassName = CLASS_NAME,
        .hCursor = LoadCursorW(0, IDC_ARROW)
    };
    if (!RegisterClassW(&wnd_class)) return plat;  // log

    HWND window = CreateWindowExW(
        0, CLASS_NAME, L"Capymilk", WS_TILEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, 1024, 1024, NULL, NULL, mod_handle,
        NULL
    );
    if (!window) return plat;  // log

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
    if (!gl_ctx) return plat;  // log
    b32 ok = wglMakeCurrent(dc, gl_ctx);
    if (!ok) return plat;  // log
    
    plat.mod_handle = mod_handle;
    plat.window = window;
    plat.device_ctx = dc;
    plat.gl_ctx = gl_ctx;

    return plat;
}

internal char *platform_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;

    fseek(f, 0L, SEEK_END);
    i32 size = (i32)ftell(f);
    rewind(f);

    char *buf = (char *)malloc(size + 1);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }

    i32 read = (i32)fread(buf, 1, size, f);
    fclose(f);

    if (read != size) {
        free(buf);
        return NULL; 
    }

    buf[size] = '\0';
    return buf;
}

internal LRESULT platform_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    app *state = (app*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!state) return DefWindowProcW(hwnd, umsg, wparam, lparam);

    i32 num_particles = state->galaxy_params->num_stars + state->galaxy_params->num_dust * 2;
    switch (umsg) {
        case WM_KEYDOWN: {
            switch (wparam) {
                case 'Q': state->galaxy_params->angular_offset += 0.00005f;    break;
                case 'A': state->galaxy_params->angular_offset -= 0.00005f;    break;
                case 'W': state->galaxy_params->ex_inner       += 0.02f;       break;
                case 'S': state->galaxy_params->ex_inner       -= 0.02f;       break;
                case 'E': state->galaxy_params->ex_outer       += 0.02f;       break;
                case 'D': state->galaxy_params->ex_outer       -= 0.02f;       break;
                case 'R': state->galaxy_params->time_scale     += 1000000.0f;  break;
                case 'F': state->galaxy_params->time_scale     -= 1000000.0f;  break;
                case 'T': { 
                    state->galaxy_params->num_stars += 1000; 
                    num_particles = state->galaxy_params->num_stars + state->galaxy_params->num_dust * 2;
                    state->vertices = (vertex*)realloc(state->vertices, num_particles * sizeof(vertex));
                    renderer_resize(state->renderer, num_particles);
                } break;
                case 'G': { 
                    state->galaxy_params->num_stars -= 1000; 
                    if (state->galaxy_params->num_stars < 0) state->galaxy_params->num_stars = 0;
                    num_particles = state->galaxy_params->num_stars + state->galaxy_params->num_dust * 2;
                    state->vertices = (vertex*)realloc(state->vertices, num_particles * sizeof(vertex));
                    renderer_resize(state->renderer, num_particles);
                } break;
                case 'Y': { 
                    state->galaxy_params->num_dust += 1000; 
                    num_particles = state->galaxy_params->num_stars + state->galaxy_params->num_dust * 2;
                    state->vertices = (vertex*)realloc(state->vertices, num_particles * sizeof(vertex));
                    renderer_resize(state->renderer, num_particles);
                } break;
                case 'H': { 
                    state->galaxy_params->num_dust -= 1000; 
                    if (state->galaxy_params->num_dust < 0) state->galaxy_params->num_dust = 0;
                    num_particles = state->galaxy_params->num_stars + state->galaxy_params->num_dust * 2;
                    state->vertices = (vertex*)realloc(state->vertices, num_particles * sizeof(vertex));
                    renderer_resize(state->renderer, num_particles);
                } break;
                case 'U': state->galaxy_params->rad_core        += 100.0f;      break;
                case 'J': state->galaxy_params->rad_core        -= 100.0f;      break;
                case 'I': state->galaxy_params->rad_galaxy      += 100.0f;      break;
                case 'K': state->galaxy_params->rad_galaxy      -= 100.0f;      break;

                default: return DefWindowProcW(hwnd, umsg, wparam, lparam);
            }
            galaxy_init(state->galaxy_params, state->vertices);
            renderer_upload(state->renderer, state->vertices, num_particles);
            printf("offset: %f ex_inner: %f ex_outer: %f time_scale %.0f num_stars %d num_dust %d rad_core %.0f rad_galaxy %.0f\n",
                state->galaxy_params->angular_offset,
                state->galaxy_params->ex_inner, 
                state->galaxy_params->ex_outer,
                state->galaxy_params->time_scale,
                state->galaxy_params->num_stars,
                state->galaxy_params->num_dust,
                state->galaxy_params->rad_core,
                state->galaxy_params->rad_galaxy);
        } break;
        case WM_SIZE: {
            u32 width = LOWORD(lparam);
            u32 height = HIWORD(lparam);
            glViewport(0, 0, width, height);

            renderer_draw(state->renderer, num_particles);

            SwapBuffers(state->platform->device_ctx);
        } break;
        case WM_CLOSE: {
            state->is_running = false;
        } break;
    }

    return DefWindowProcW(hwnd, umsg, wparam, lparam);
}

internal renderer renderer_create(i32 n) {
    renderer render = {0};

    GLuint shader_program = 0;
    {
        i32 ok = 0;
        char info_log[512];

        char *vert_source = platform_read_file("galaxy.vert");
        if (vert_source == NULL) {
            printf("failed to load galaxy.vert\n");
            return render;
        }

        char *frag_source = platform_read_file("galaxy.frag");
        if (frag_source == NULL) {
            printf("failed to load galaxy.frag\n");
            free(vert_source);
            return render;
        }

        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, (const char**)&vert_source, NULL);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
            printf("vertex shader failed to compile: %s\n", info_log);
            free(vert_source);
            free(frag_source);
            glDeleteShader(vertex_shader);
            return render;
        }

        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, (const char**)&frag_source, NULL);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
            printf("fragment shader failed to compile: %s\n", info_log);
            free(vert_source);
            free(frag_source);
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            return render;
        }

        shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
        glGetProgramiv(shader_program, GL_LINK_STATUS, &ok);
        if (!ok) {
            glGetProgramInfoLog(shader_program, 512, NULL, info_log);
            printf("shader program failed to link: %s\n", info_log);
            free(vert_source);
            free(frag_source);
            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            return render;
        }

        free(vert_source);
        free(frag_source);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    GLuint vao = 0;
    {
        glCreateVertexArrays(1, &vao);

        // color
        glVertexArrayAttribBinding(vao, 0, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vao, 0);

        // radius
        glVertexArrayAttribBinding(vao, 1, 0);
        glVertexArrayAttribFormat(vao, 1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 1);

        // theta
        glVertexArrayAttribBinding(vao, 2, 0);
        glVertexArrayAttribFormat(vao, 2, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 2);

        // ang_speed
        glVertexArrayAttribBinding(vao, 3, 0);
        glVertexArrayAttribFormat(vao, 3, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 3);

        // mag
        glVertexArrayAttribBinding(vao, 4, 0);
        glVertexArrayAttribFormat(vao, 4, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 4);

        // type
        glVertexArrayAttribBinding(vao, 5, 0);
        glVertexArrayAttribFormat(vao, 5, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 5);
    }

    render.shader_program = shader_program;
    render.vao = vao;
    renderer_resize(&render, n);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    return render;
}

internal void renderer_resize(renderer *render, i32 n) {
    if (render->vbo) glDeleteBuffers(1, &render->vbo);

    glCreateBuffers(1, &render->vbo);
    glNamedBufferStorage(render->vbo, sizeof(vertex) * n, NULL, GL_DYNAMIC_STORAGE_BIT);
    glVertexArrayVertexBuffer(render->vao, 0, render->vbo, 0, sizeof(vertex));
}

internal void renderer_draw(const renderer *render, i32 n) {
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(render->vao);
    glUseProgram(render->shader_program);
    glDrawArrays(GL_POINTS, 0, n);
}

internal void renderer_upload(renderer *render, vertex *vertices, i32 n) {
    glNamedBufferSubData(render->vbo, 0, sizeof(vertex) * n, vertices);
}

internal void renderer_update(renderer *render, galaxy_params *params, f32 elapsed_years) {
    GLuint shader = render->shader_program;
    glProgramUniform1f(shader, glGetUniformLocation(shader, "u_elapsed_years"),  elapsed_years);
    glProgramUniform1f(shader, glGetUniformLocation(shader, "u_ex_inner"),       params->ex_inner);
    glProgramUniform1f(shader, glGetUniformLocation(shader, "u_ex_outer"),       params->ex_outer);
    glProgramUniform1f(shader, glGetUniformLocation(shader, "u_rad_core"),       params->rad_core);
    glProgramUniform1f(shader, glGetUniformLocation(shader, "u_rad_galaxy"),     params->rad_galaxy);
    glProgramUniform1f(shader, glGetUniformLocation(shader, "u_angular_offset"), params->angular_offset);
}

internal f32 galaxy_ex(const galaxy_params *params, f32 radius) {
    f32 rad_farfield = params->rad_galaxy * 2;
    if (radius < params->rad_core) {
        f32 t = radius / params->rad_core;
        return 1.0f + t * (params->ex_inner - 1.0f);
    } else if (radius < params->rad_galaxy) {
        f32 t = (radius - params->rad_core) / (params->rad_galaxy - params->rad_core);
        return params->ex_inner + t * (params->ex_outer - params->ex_inner);
    } else if (radius < rad_farfield) {
        f32 t = (radius - params->rad_galaxy) / (rad_farfield - params->rad_galaxy);
        return params->ex_outer + t * (1.0f - params->ex_outer);
    } else return 1.0f;
}

internal f32 galaxy_brightness(f32 rad_core, f32 rad_galaxy, f32 radius) {
    f32 a = rad_galaxy / 3.0f;
    if (radius < rad_core) {
        return I0_BULGE * expf(-K * powf(radius, 0.25f));
    } else {
        f32 i0_disk = I0_BULGE * expf(-K * powf(rad_core, 0.25f));
        return i0_disk * expf(-(radius - rad_core) / a);
    }
}

internal void galaxy_cdf_build(f32 *radii, f32 *cumulative, f32 *inverse, f32 rad_core, f32 rad_galaxy) {
    f32 rad_farfield = rad_galaxy * 2;
    f32 step = rad_farfield / CDF_STEPS;
    f32 h = 1.0f / (CDF_POINTS - 1);

    f32 total = 0;
    for (i32 i = 0; i < CDF_POINTS; i++) {
        f32 l = (i*2) * step;
        f32 m = (i*2 + 1) * step;
        f32 r = (i*2 + 2) * step;
        total += step / 3 * (galaxy_brightness(rad_core, rad_galaxy, l) +
                             galaxy_brightness(rad_core, rad_galaxy, m) * 4 +
                             galaxy_brightness(rad_core, rad_galaxy, r));

        radii[i] = r;
        cumulative[i] = total;
    }

    // normalize
    f32 max = cumulative[CDF_POINTS - 1];
    for (i32 i = 0; i < CDF_POINTS; i++) {
        cumulative[i] /= max;
    }

    for (i32 i = 0; i < CDF_POINTS; i++) {
        f32 p = i * h;

        i32 lo = 0, hi = CDF_POINTS - 1;
        while (lo < hi) {
            i32 mid = lo + (hi - lo) / 2;
            if (p > cumulative[mid]) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        if (lo == 0) {
            inverse[i] = radii[0];
        }
        else {
            f32 t = (p - cumulative[lo-1]) / (cumulative[lo] - cumulative[lo-1]);
            inverse[i] = radii[lo-1] + t * (radii[lo] - radii[lo-1]);
        }
    }
}

internal f32 galaxy_cdf_sample(const f32 *inverse, f32 p) {
    f32 h = 1.0f / (CDF_POINTS - 1);
    i32 i = (i32)(p / h);
    if (i >= CDF_POINTS - 1)
        return inverse[CDF_POINTS-1];

    f32 t = (p - i * h) / h;
    return inverse[i] + t * (inverse[i+1] - inverse[i]);
}

internal f32 galaxy_bb_spectrum(i32 wavelength, f32 temp) {
    f32 wavelength_m = wavelength * 1e-9f;
    return PLANCK_C1 / powf(wavelength_m, 5) / (expf(PLANCK_C2 / (wavelength_m * temp)) - 1);
}

internal vec3 galaxy_spectrum_to_xyz(f32 temp) {
    local_persist const f32 cie_color_match[81][3] = {
        {0.0014f,0.0000f,0.0065f}, {0.0022f,0.0001f,0.0105f}, {0.0042f,0.0001f,0.0201f},
        {0.0076f,0.0002f,0.0362f}, {0.0143f,0.0004f,0.0679f}, {0.0232f,0.0006f,0.1102f},
        {0.0435f,0.0012f,0.2074f}, {0.0776f,0.0022f,0.3713f}, {0.1344f,0.0040f,0.6456f},
        {0.2148f,0.0073f,1.0391f}, {0.2839f,0.0116f,1.3856f}, {0.3285f,0.0168f,1.6230f},
        {0.3483f,0.0230f,1.7471f}, {0.3481f,0.0298f,1.7826f}, {0.3362f,0.0380f,1.7721f},
        {0.3187f,0.0480f,1.7441f}, {0.2908f,0.0600f,1.6692f}, {0.2511f,0.0739f,1.5281f},
        {0.1954f,0.0910f,1.2876f}, {0.1421f,0.1126f,1.0419f}, {0.0956f,0.1390f,0.8130f},
        {0.0580f,0.1693f,0.6162f}, {0.0320f,0.2080f,0.4652f}, {0.0147f,0.2586f,0.3533f},
        {0.0049f,0.3230f,0.2720f}, {0.0024f,0.4073f,0.2123f}, {0.0093f,0.5030f,0.1582f},
        {0.0291f,0.6082f,0.1117f}, {0.0633f,0.7100f,0.0782f}, {0.1096f,0.7932f,0.0573f},
        {0.1655f,0.8620f,0.0422f}, {0.2257f,0.9149f,0.0298f}, {0.2904f,0.9540f,0.0203f},
        {0.3597f,0.9803f,0.0134f}, {0.4334f,0.9950f,0.0087f}, {0.5121f,1.0000f,0.0057f},
        {0.5945f,0.9950f,0.0039f}, {0.6784f,0.9786f,0.0027f}, {0.7621f,0.9520f,0.0021f},
        {0.8425f,0.9154f,0.0018f}, {0.9163f,0.8700f,0.0017f}, {0.9786f,0.8163f,0.0014f},
        {1.0263f,0.7570f,0.0011f}, {1.0567f,0.6949f,0.0010f}, {1.0622f,0.6310f,0.0008f},
        {1.0456f,0.5668f,0.0006f}, {1.0026f,0.5030f,0.0003f}, {0.9384f,0.4412f,0.0002f},
        {0.8544f,0.3810f,0.0002f}, {0.7514f,0.3210f,0.0001f}, {0.6424f,0.2650f,0.0000f},
        {0.5419f,0.2170f,0.0000f}, {0.4479f,0.1750f,0.0000f}, {0.3608f,0.1382f,0.0000f},
        {0.2835f,0.1070f,0.0000f}, {0.2187f,0.0816f,0.0000f}, {0.1649f,0.0610f,0.0000f},
        {0.1212f,0.0446f,0.0000f}, {0.0874f,0.0320f,0.0000f}, {0.0636f,0.0232f,0.0000f},
        {0.0468f,0.0170f,0.0000f}, {0.0329f,0.0119f,0.0000f}, {0.0227f,0.0082f,0.0000f},
        {0.0158f,0.0057f,0.0000f}, {0.0114f,0.0041f,0.0000f}, {0.0081f,0.0029f,0.0000f},
        {0.0058f,0.0021f,0.0000f}, {0.0041f,0.0015f,0.0000f}, {0.0029f,0.0010f,0.0000f},
        {0.0020f,0.0007f,0.0000f}, {0.0014f,0.0005f,0.0000f}, {0.0010f,0.0004f,0.0000f},
        {0.0007f,0.0002f,0.0000f}, {0.0005f,0.0002f,0.0000f}, {0.0003f,0.0001f,0.0000f},
        {0.0002f,0.0001f,0.0000f}, {0.0002f,0.0001f,0.0000f}, {0.0001f,0.0000f,0.0000f},
        {0.0001f,0.0000f,0.0000f}, {0.0001f,0.0000f,0.0000f}, {0.0000f,0.0000f,0.0000f}
    };

    vec3 xyz = {0};
    for (i32 i = 0, lambda = 380; i < 81; i++, lambda += 5) {
        f32 intensity = galaxy_bb_spectrum(lambda, temp);
        xyz.x += intensity * cie_color_match[i][0];
        xyz.y += intensity * cie_color_match[i][1];
        xyz.z += intensity * cie_color_match[i][2];
    }

    f32 total = xyz.x + xyz.y + xyz.z;
    xyz.x /= total;
    xyz.y /= total;
    xyz.z /= total;

    return xyz;
}

internal vec3 galaxy_xyz_to_rgb(vec3 xyz) {
    local_persist const mat3 srgb = {{
     3.2406f, -1.5372f, -0.4986f,
    -0.9689f,  1.8758f,  0.0415f,
     0.0557f, -0.2040f,  1.0570f
    }};

    return mat3_mul_vec3(srgb, xyz);
}

internal vec3 galaxy_bb_color(f32 temp) {
    vec3 xyz = galaxy_spectrum_to_xyz(temp);
    vec3 rgb = galaxy_xyz_to_rgb(xyz);
    
    // constrain
    f32 w = MIN(0, MIN(rgb.x, MIN(rgb.y, rgb.z)));
    if (w < 0) {
        rgb.x -= w;
        rgb.y -= w;
        rgb.z -= w;
    }

    // normalize
    f32 m = MAX(rgb.x, MAX(rgb.y, rgb.z));
    if (m > 0) {
        rgb.x /= m;
        rgb.y /= m;
        rgb.z /= m;
    }

    return rgb;
}

internal void galaxy_ct_build(void) {
    f32 step = (TEMP_MAX - TEMP_MIN) / COLOR_TABLE_SIZE;
    for (i32 i = 0; i < COLOR_TABLE_SIZE; i++) {
        f32 temp = TEMP_MIN + i * step;
        vec3 rgb = galaxy_bb_color(temp);
        color_table[i] = rgb;
    }
}

internal vec3 galaxy_ct_lookup(f32 temp) {
    f32 step = (TEMP_MAX - TEMP_MIN) / COLOR_TABLE_SIZE;
    i32 i = ((temp - TEMP_MIN) / step);
    i = MAX(0, MIN(COLOR_TABLE_SIZE - 1, i)); // clamp

    return color_table[i];
}

internal f32 galaxy_mass_disc(f32 rad_core, f32 r) {
    f32 disc_falloff = rad_core / 3.0f;
    f32 disc_thickness = 2000.0f;
    f32 disc_density_center = 1.0f;

    return disc_density_center * expf(-r / disc_falloff) * (r * r) * PI * disc_thickness;
}

internal f32 galaxy_mass_halo(f32 rad_galaxy, f32 r) {
    f32 halo_falloff = rad_galaxy / 6.0f;
    f32 halo_density_center = 0.15f;

    return halo_density_center * (1.0f / (1.0f + powf(r / halo_falloff, 2.0f))) * (4.0f * PI * powf(r, 3.0f) / 3.0f);
}

internal f32 galaxy_velocity_kms(f32 rad_core, f32 rad_galaxy, f32 r) {
    f32 mass_buffer = 100.0f;
    return 20000.0f * sqrtf(G * (galaxy_mass_halo(rad_galaxy, r) + galaxy_mass_disc(rad_core, r) + mass_buffer) / r) ;
}

internal f32 galaxy_orbital_velocity(f32 rad_core, f32 rad_galaxy, f32 r) {
    f32 v = galaxy_velocity_kms(rad_core, rad_galaxy, r);
    f32 circ = 2.0f * PI * r * PC_TO_KM;
    f32 time = circ / (v * SEC_PER_YEAR);

    return (360.0f / time) * (PI / 180.0f);
}

internal void galaxy_init(galaxy_params *params, vertex *vertices) {
    f32 radii[CDF_POINTS], cumulative[CDF_POINTS], inverse[CDF_POINTS];
    galaxy_cdf_build(radii, cumulative, inverse, params->rad_core, params->rad_galaxy);

    f32 rand_scale = 1.0f / RAND_MAX;
    for (i32 i = 0; i < params->num_stars; i++) {
        f32 radius = galaxy_cdf_sample(inverse, rand() * rand_scale);
        f32 theta0 = (rand() * rand_scale) * 2.0f * PI;
        f32 ang_speed = galaxy_orbital_velocity(params->rad_core, params->rad_galaxy, radius);

        f32 temp = 6000 + (4000 * rand() * rand_scale - 2000);
        vec3 color = galaxy_ct_lookup(temp);

        vertices[i].color.x = color.x;
        vertices[i].color.y = color.y;
        vertices[i].color.z = color.z;
        vertices[i].radius = radius;
        vertices[i].theta = theta0;
        vertices[i].ang_speed = ang_speed;
        vertices[i].mag = 0.1f + 0.4f * (rand() * rand_scale);
        vertices[i].type = 0.0f;
    }

    for (i32 i = 0; i < params->num_dust; i++) {
        i32 idx = params->num_stars + i;
        f32 radius;
        if (i % 2 == 0)
            radius = galaxy_cdf_sample(inverse, rand() * rand_scale);
        else {
            f32 x = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
            f32 y = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
            radius = sqrt(x * x + y * y);
        }

        f32 theta = (rand() * rand_scale) * 2.0f * PI;
        f32 b = radius * galaxy_ex(params, radius);
        f32 avg_radius = (radius + b) / 2.0f;
        f32 ang_speed = galaxy_orbital_velocity(params->rad_core, params->rad_galaxy, avg_radius);

        f32 temp = 4000 + radius / 4.0f;
        vec3 color = galaxy_ct_lookup(temp);

        vertices[idx].color.x = color.x;
        vertices[idx].color.y = color.y;
        vertices[idx].color.z = color.z;
        vertices[idx].radius = radius;
        vertices[idx].theta = theta;
        vertices[idx].ang_speed = ang_speed;
        vertices[idx].mag = 0.02f + 0.15f * (rand() * rand_scale);
        vertices[idx].type = 1.0f;
    }

    i32 idx = params->num_stars + params->num_dust;
    for (i32 i = 0; i < params->num_dust / 100; i++) {
        f32 x = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
        f32 y = 2 * params->rad_galaxy * (rand() * rand_scale) - params->rad_galaxy;
        f32 origin = sqrt(x * x + y * y);

        f32 origin_theta =  (rand() * rand_scale) * 2.0f * PI;
        f32 mag = 0.1f + 0.05f * (rand() * rand_scale);
        
        i32 n = (i32)(100 * (rand() * rand_scale));
        for (i32 j = 0; j < n; j++) {
            f32 radius = origin + 200 - 400 * (rand() * rand_scale);
            f32 theta = origin_theta + (10.0f - 20.0f * (rand() * rand_scale)) * (PI / 180.0f);
            f32 b = radius * galaxy_ex(params, radius);
            f32 avg_radius = (radius + b) / 2.0f;
            f32 ang_speed = galaxy_orbital_velocity(params->rad_core, params->rad_galaxy, avg_radius);

            f32 temp = 4000 + radius / 4.5f - 1000;
            vec3 color = galaxy_ct_lookup(temp);

            vertices[idx].color.x = color.x;
            vertices[idx].color.y = color.y;
            vertices[idx].color.z = color.z;
            vertices[idx].radius = radius;
            vertices[idx].theta = theta;
            vertices[idx].ang_speed = ang_speed;
            vertices[idx].mag = mag + 0.025f * (rand() * rand_scale);
            vertices[idx].type = 2.0f;
            idx++;
        }
    }
}
