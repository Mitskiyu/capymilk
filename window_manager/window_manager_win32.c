#include "window_manager_win32.h"
#include <windowsx.h>

internal wm_window *wm_window_open(mem_arena *arena, u32 w, u32 h, const char *title) {
    wm_window *window = PUSH_STRUCT(arena, wm_window);

    window->plat_info = PUSH_STRUCT(arena, _wm_plat_info);

    HINSTANCE mod_handle = GetModuleHandle(NULL);

    const wchar_t *class_name = L"CAPYMILK";
    int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t *wide = PUSH_ARRAY(arena, wchar_t, len); // want temp arena
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wide, len);

    WNDCLASSW wnd_class = {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = _wm_window_proc,
        .hInstance = mod_handle,
        .lpszClassName = class_name,
        .hCursor = LoadCursorW(0, IDC_ARROW)
    };
    if (!RegisterClassW(&wnd_class)) { 
        plat_fatal_err("failed to register window class", 1);
    }

    RECT rect = { 0, 0, w, h };
    AdjustWindowRectEx(&rect, WS_TILEDWINDOW, FALSE, 0);
    i32 win_w = rect.right - rect.left;
    i32 win_h = rect.bottom - rect.top;
    HWND win_handle = CreateWindowExW(
        0, class_name, wide, WS_TILEDWINDOW | WS_VISIBLE, CW_USEDEFAULT,
        CW_USEDEFAULT, win_w, win_h, NULL, NULL, mod_handle, NULL
    );
    if (!win_handle) {
        plat_fatal_err("failed to create window", 1);
    }

    window->width  = w;
    window->height = h;
    window->plat_info->win_handle = win_handle;
    window->plat_info->mod_handle = mod_handle;
    SetWindowLongPtrW(win_handle, GWLP_USERDATA, (LONG_PTR)window);

    return window;
}

internal void wm_window_process_events(wm_window *window) {
    memcpy(window->prev_keys, window->keys, sizeof(window->keys));
    memcpy(window->prev_mouse_buttons, window->mouse_buttons, sizeof(window->mouse_buttons));

    MSG msg = {0};
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

internal void wm_window_set_draw_callback(wm_window *window, wm_draw_fn fn, void *ctx) {
    window->draw_callback = fn;
    window->draw_ctx = ctx;
}

internal LRESULT CALLBACK _wm_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
    wm_window *window = (wm_window*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (!window) return DefWindowProcW(hwnd, umsg, wparam, lparam);

    switch (umsg) {
        case WM_KEYDOWN: {
            if (wparam < sizeof(window->keys) / sizeof(window->keys[0]))
                window->keys[wparam] = true;
        } break;

        case WM_KEYUP: {
            if (wparam < sizeof(window->keys) / sizeof(window->keys[0]))
                window->keys[wparam] = false;
        } break;

        case WM_MOUSEMOVE: {
            window->mouse_pos.x = (f32)GET_X_LPARAM(lparam);
            window->mouse_pos.y = (f32)GET_Y_LPARAM(lparam);
        } break;

        case WM_LBUTTONDOWN: {
            window->mouse_buttons[WM_MB_LEFT] = true;
            window->mouse_pos.x = (f32)GET_X_LPARAM(lparam);
            window->mouse_pos.y = (f32)GET_Y_LPARAM(lparam);
        } break;

        case WM_LBUTTONUP: {
            window->mouse_buttons[WM_MB_LEFT] = false;
            window->mouse_pos.x = (f32)GET_X_LPARAM(lparam);
            window->mouse_pos.y = (f32)GET_Y_LPARAM(lparam);
        } break;

        case WM_RBUTTONDOWN: {
            window->mouse_buttons[WM_MB_RIGHT] = true;
            window->mouse_pos.x = (f32)GET_X_LPARAM(lparam);
            window->mouse_pos.y = (f32)GET_Y_LPARAM(lparam);
        } break;

        case WM_RBUTTONUP: {
            window->mouse_buttons[WM_MB_RIGHT] = false;
            window->mouse_pos.x = (f32)GET_X_LPARAM(lparam);
            window->mouse_pos.y = (f32)GET_Y_LPARAM(lparam);
        } break;

        case WM_SIZE: {
            window->width  = (u32)LOWORD(lparam);
            window->height = (u32)HIWORD(lparam);
            if (window->draw_callback)
                window->draw_callback(window->draw_ctx);
        } break;

        case WM_ENTERSIZEMOVE: {
            SetTimer(hwnd, 1, 10, NULL);
        } break;

        case WM_EXITSIZEMOVE: {
            KillTimer(hwnd, 1);
        } break;

        case WM_TIMER: {
            if (window->draw_callback)
                window->draw_callback(window->draw_ctx);
        } break;

        case WM_CLOSE: {
            window->should_close = true;
        } break;

        default: return DefWindowProcW(hwnd, umsg, wparam, lparam);
    }

    return 0;
}
