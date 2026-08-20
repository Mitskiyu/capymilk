#ifndef BASE_DEFS_H
#define BASE_DEFS_H

#if defined(_WIN32)
#   define PLATFORM_WIN32 1
#elif defined(__APPLE__) && defined(__MACH__)
#   define PLATFORM_MACOS 1
#elif defined(__linux__)
#   define PLATFORM_LINUX 1
#endif

#if !defined(PLATFORM_WIN32)
#   define PLATFORM_WIN32 0
#endif 
#if !defined(PLATFORM_MACOS)
#   define PLATFORM_MACOS 0
#endif
#if !defined(PLATFORM_LINUX)
#   define PLATFORM_LINUX 0
#endif

#define internal      static
#define global        static
#define local_persist static

#define PI 3.14159265358979f

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8  b8;
typedef i32 b32;

typedef float  f32;
typedef double f64;

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

#define ALIGN_UP_POW2(n, p) (((u64)(n) + (u64)(p) - 1) & ~((u64)(p) - 1))
#define ALIGN_DOWN_POW2(n, p) ((u64)(n) & ~((u64)(p) - 1))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif
