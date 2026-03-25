typedef char GLchar;
typedef intptr_t GLintptr;
typedef i64 GLsizeiptr;

#define GL_ARRAY_BUFFER    0x8892 // https://registry.khronos.org/OpenGL/api/GL/glext.h
#define GL_STATIC_DRAW     0x88E4
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82

#ifdef _WIN32
#   define OPENGL_CALLSTYLE __stdcall
#else
#   define OPENGL_CALLSTYLE
#endif

#define X(ret, name, args) \
    typedef ret OPENGL_CALLSTYLE gl_##name##_func args; \
    static gl_##name##_func* gl##name;
#include "opengl_xlist.h"
#undef X
