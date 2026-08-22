#include <windows.h>
#include <winuser.h>

global u64 _w32_perf_freq = 1;

internal void plat_init(void) {
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
        _w32_perf_freq = (u64)freq.QuadPart;
    }
}

internal void plat_fatal_err(const char *msg, i32 code) {
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
    ExitProcess((u32)code);
}

internal u64 plat_time_usec(void) {
    LARGE_INTEGER ticks = {0};
    if (!QueryPerformanceCounter(&ticks)) return 0;
    return (u64)ticks.QuadPart * 1000000 / _w32_perf_freq;
}

// returns NULL on failure
internal void *plat_mem_reserve(u64 size) {
    return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

internal b32 plat_mem_commit(void *mem, u64 size) {
    void *ret = VirtualAlloc(mem, size, MEM_COMMIT, PAGE_READWRITE);
    return ret != NULL;
}

internal b32 plat_mem_release(void *mem, u64 size) {
    return VirtualFree(mem, size, MEM_RELEASE);
}

internal b32 plat_mem_decommit(void *mem, u64 size) {
    return VirtualFree(mem, size, MEM_DECOMMIT);
}

internal u32 plat_page_size(void) {
    SYSTEM_INFO si = { 0 };
    GetSystemInfo(&si);
    return si.dwPageSize;
}
