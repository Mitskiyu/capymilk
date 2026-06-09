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

#define NUM_STARS     60000
#define NUM_DUST      NUM_STARS
#define NUM_PARTICLES (NUM_STARS + NUM_DUST)

#define RAD_GALAXY   15000.0f
#define RAD_CORE     6000.0f
#define RAD_FARFIELD (RAD_GALAXY * 2.0f)

#define I0_BULGE   1.0f
#define K          0.02f
#define A          (RAD_GALAXY / 3.0f)

#define CDF_STEPS  1000
#define CDF_POINTS (CDF_STEPS / 2)

#define PLANCK_C1 3.74183e-16f
#define PLANCK_C2 1.4388e-2f

typedef struct {
    f32 x, y, z;
    f32 r, g, b;
    f32 mag;
    f32 type;
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
    f32 angular_offset;
    f32 ex_inner;
    f32 ex_outer;
} galaxy_params;

typedef struct {
    b32           is_running;
    vertex        *vertices;
    platform      *platform;
    renderer      *renderer;
    galaxy_params *galaxy_params;
} app;

internal platform platform_create(void);
internal LRESULT platform_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
internal renderer renderer_create(void);
internal void renderer_draw(renderer *render);
internal void renderer_upload(renderer *render, vertex *vertices);
internal f32 galaxy_ex(galaxy_params *params, f32 radius);
internal f32 galaxy_brightness(f32 radius);
internal void galaxy_cdf_build(f32 *radii, f32 *cumulative);
internal f32 galaxy_cdf_sample(f32 *radii, f32 *cumulative, f32 t);
internal f32 galaxy_bb_spectrum(i32 wavelength, f32 temp);
internal vec3 galaxy_spectrum_to_xyz(f32 temp);
internal vec3 galaxy_xyz_to_rgb(vec3 xyz);
internal vec3 galaxy_bb_color(f32 temp);
internal void galaxy_generate(galaxy_params *params, vertex *vertices);

global const char *vert_shader_source =
    "#version 450\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec3 a_color;\n"
    "layout (location = 2) in float a_mag;\n"
    "layout (location = 3) in float a_type;\n"
    "out vec3 v_color;\n"
    "flat out float v_type;\n"
    "void main()\n"
    "{\n"
    "gl_Position = vec4(a_pos, 1.0);\n"
    "if (a_type == 0.0) {\n"
        "gl_PointSize = 4.0 * a_mag;\n"
    "}\n"
    "else {\n"
        "gl_PointSize = 5.0 * 70.0 * a_mag;\n"
    "}\n"
    "v_color = a_color * a_mag;\n"
    "v_type = a_type;\n"
    "}\n\0";

global const char *frag_shader_source =
    "#version 450\n"
    "in vec3 v_color;\n"
    "flat in float v_type;\n"
    "out vec4 frag_color;\n"
    "void main()\n"
    "{\n"
    "vec2 circ = 2.0 * gl_PointCoord - 1.0;\n"
    "float alpha;\n"
    "if (v_type == 0.0) {\n"
        "alpha = 1.0 - length(circ);\n"
    "}\n"
    "else {\n"
        "alpha = 0.05 * (1.0 - length(circ));\n"
    "}\n"
    "frag_color = vec4(v_color, alpha);\n"
    "}\n\0";

int main(void) {
    srand((u32)time(NULL));

    platform plat = platform_create();
    if (!plat.window) return 1;

    vertex *vertices = (vertex *)malloc(NUM_PARTICLES * sizeof(vertex));
    if (!vertices) return 1;

    renderer render = renderer_create();
    galaxy_params params = {-0.0004f, 0.8f, 1.0f};
    app state = {
        .is_running = true,
        .vertices = vertices,
        .platform = &plat,
        .renderer = &render,
        .galaxy_params = &params,
    };

    SetWindowLongPtrW(plat.window, GWLP_USERDATA, (LONG_PTR)&state);

    galaxy_generate(&params, vertices);
    renderer_upload(&render, vertices);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0);
    while (state.is_running) {
        MSG msg = {0};
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Draw
        renderer_draw(&render);

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

internal renderer renderer_create(void) {
    renderer render = {0};

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

        glNamedBufferStorage(vbo, sizeof(vertex) * NUM_PARTICLES, NULL, GL_DYNAMIC_STORAGE_BIT);
        glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(vertex));

        // pos
        glVertexArrayAttribBinding(vao, 0, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vao, 0);

        // color
        glVertexArrayAttribBinding(vao, 1, 0);
        glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 1);

        // magnitude
        glVertexArrayAttribBinding(vao, 2, 0);
        glVertexArrayAttribFormat(vao, 2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 2);

        // type
        glVertexArrayAttribBinding(vao, 3, 0);
        glVertexArrayAttribFormat(vao, 3, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(f32));
        glEnableVertexArrayAttrib(vao, 3);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    render.shader_program = shader_program;
    render.vao = vao;
    render.vbo = vbo;

    return render;
}

internal void renderer_draw(renderer *render) {
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(render->vao);
    glUseProgram(render->shader_program);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
}

internal void renderer_upload(renderer *render, vertex *vertices) {
    glNamedBufferSubData(render->vbo, 0, sizeof(vertex) * NUM_PARTICLES, vertices);
}

internal f32 galaxy_ex(galaxy_params *params, f32 radius) {
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
    if (radius < RAD_CORE) {
        return I0_BULGE * expf(-K * powf(radius, 0.25f));
    } else {
        f32 i0_disk = I0_BULGE * expf(-K * powf(RAD_CORE, 0.25f));
        return i0_disk * expf(-(radius - RAD_CORE) / A);
    }
}

internal void galaxy_cdf_build(f32 *radii, f32 *cumulative) {
    f32 step = (RAD_FARFIELD) / CDF_STEPS;

    f32 total = 0;
    for (i32 i = 0; i < CDF_POINTS; i++) {
        f32 l = (i*2) * step;
        f32 m = (i*2 + 1) * step;
        f32 r = (i*2 + 2) * step;
        total += step / 3 * (galaxy_brightness(l) +
                             galaxy_brightness(m) * 4 +
                             galaxy_brightness(r));

        radii[i] = r;
        cumulative[i] = total;
    }

    f32 max = cumulative[CDF_POINTS - 1];
    for (i32 i = 0; i < CDF_POINTS; i++) {
        cumulative[i] /= max;
    }
}

internal f32 galaxy_cdf_sample(f32 *radii, f32 *cumulative, f32 t) {
    i32 i = 0;
    while (t > cumulative[i] && i < CDF_POINTS - 1) {
        i++;
    }

    return radii[i];
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

internal void galaxy_generate(galaxy_params *params, vertex *vertices) {
    f32 radii[CDF_POINTS];
    f32 cumulative[CDF_POINTS];
    galaxy_cdf_build(radii, cumulative);

    f32 scale = 0.5f / RAD_GALAXY;
    f32 rand_scale = 1.0f / RAND_MAX;
    for (i32 i = 0; i < NUM_STARS; i++) {
        f32 radius = galaxy_cdf_sample(radii, cumulative, rand() * rand_scale);
        f32 tilt = radius * params->angular_offset;
        f32 theta = (rand() * rand_scale) * 2.0f * PI;
        f32 x = cosf(theta) * radius;
        f32 y = sinf(theta) * radius * galaxy_ex(params, radius);
        f32 cos_r = cosf(tilt);
        f32 sin_r = sinf(tilt);

        f32 temp = 6000 + (4000 * rand() * rand_scale - 2000);
        vec3 color = galaxy_bb_color(temp);

        vertices[i].x = (x * cos_r - y * sin_r) * scale;
        vertices[i].y = (x * sin_r + y * cos_r) * scale;
        vertices[i].r = color.x;
        vertices[i].g = color.y;
        vertices[i].b = color.z;
        vertices[i].mag = 0.1f + 0.4f * (rand() * rand_scale);
        vertices[i].type = 0.0f;
    }

    for (i32 i = 0; i < NUM_DUST; i++) {
        i32 idx = NUM_STARS + i;
        f32 radius;
        if (i % 2 == 0)
            radius = galaxy_cdf_sample(radii, cumulative, rand() * rand_scale);
        else {
            f32 x = 2 * RAD_GALAXY * (rand() * rand_scale) - RAD_GALAXY;
            f32 y = 2 * RAD_GALAXY * (rand() * rand_scale) - RAD_GALAXY;
            radius = sqrt(x * x + y * y);
        }

        f32 tilt = radius * params->angular_offset;
        f32 theta = (rand() * rand_scale) * 2.0f * PI;
        f32 x = cosf(theta) * radius;
        f32 y = sinf(theta) * radius * galaxy_ex(params, radius);
        f32 cos_r = cosf(tilt);
        f32 sin_r = sinf(tilt);

        f32 temp = 4000 + radius / 4.0f;
        vec3 color = galaxy_bb_color(temp);

        vertices[idx].x = (x * cos_r - y * sin_r) * scale;
        vertices[idx].y = (x * sin_r + y * cos_r) * scale;
        vertices[idx].r = color.x;
        vertices[idx].g = color.y;
        vertices[idx].b = color.z;
        vertices[idx].mag = 0.02f + 0.15f * (rand() * rand_scale);
        vertices[idx].type = 1.0f;
    }
}

internal LRESULT platform_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    app *state = (app*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!state) return DefWindowProcW(hwnd, umsg, wparam, lparam);

    switch (umsg) {
        case WM_KEYDOWN: {
            switch (wparam) {
                case 'Q': state->galaxy_params->angular_offset += 0.00005f; break;
                case 'A': state->galaxy_params->angular_offset -= 0.00005f; break;
                case 'W': state->galaxy_params->ex_inner       += 0.02f;  break;
                case 'S': state->galaxy_params->ex_inner       -= 0.02f;  break;
                case 'E': state->galaxy_params->ex_outer       += 0.02f;  break;
                case 'D': state->galaxy_params->ex_outer       -= 0.02f;  break;
                default: return DefWindowProcW(hwnd, umsg, wparam, lparam);
            }
            galaxy_generate(state->galaxy_params, state->vertices);
            renderer_upload(state->renderer, state->vertices);
            printf("offset: %f  ex_inner: %f ex_outer: %f\n", state->galaxy_params->angular_offset,
                                                                      state->galaxy_params->ex_inner, 
                                                                      state->galaxy_params->ex_outer);
        } break;
        case WM_SIZE: {
            u32 width = LOWORD(lparam);
            u32 height = HIWORD(lparam);
            glViewport(0, 0, width, height);

            renderer_draw(state->renderer);

            SwapBuffers(state->platform->device_ctx);
        } break;
        case WM_CLOSE: {
            state->is_running = false;
        } break;
    }

    return DefWindowProcW(hwnd, umsg, wparam, lparam);
}
