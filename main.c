#include <stdbool.h>
#include <stdint.h>

#define UNICODE
#include <windows.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef i8 b8;
typedef i32 b32;

typedef float f32;

#define CLASS_NAME L"class_name"

LRESULT window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

int main(void) {
    HINSTANCE mod_handle = GetModuleHandle(NULL);

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

    b32 is_running = true;
    SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)&is_running);

    while (is_running) {
        MSG msg = {0};
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    DestroyWindow(window);
    UnregisterClassW(CLASS_NAME, mod_handle);
    return 0;
}

static LRESULT window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    switch (umsg) {
        case WM_CLOSE: {
            b32* is_running = (b32*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (is_running) *is_running = false;
        } break;
    }

    return DefWindowProcW(hwnd, umsg, wparam, lparam);
}
