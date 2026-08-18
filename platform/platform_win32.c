#include <windows.h>

global u64 _w32_perf_freq = 1;

internal void plat_init(void) {
    LARGE_INTEGER freq;
    if (QueryPerformanceFrequency(&freq)) {
        _w32_perf_freq = (u64)freq.QuadPart;
    }
}

internal u64 plat_time_usec(void) {
    LARGE_INTEGER ticks = {0};
    if (!QueryPerformanceCounter(&ticks)) return 0;
    return (u64)ticks.QuadPart * 1000000 / _w32_perf_freq;
}
