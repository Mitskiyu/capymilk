// https://registry.khronos.org/OpenGL/api/GL/glcorearb.h
typedef unsigned int  GLenum;
typedef unsigned int  GLuint;
typedef unsigned char GLboolean;
typedef unsigned int  GLbitfield;
typedef int           GLint;
typedef int           GLsizei;
typedef float         GLfloat;
typedef char          GLchar;
typedef intptr_t      GLsizeiptr;
typedef intptr_t      GLintptr;

#define GL_FALSE           0
#define GL_TRIANGLES       0x0004
#define GL_FLOAT           0x1406
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82

// https://registr.khronos.org/OpenGL/api/GL/glext.h

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
