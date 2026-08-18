#ifndef WINDOW_MANAGER_WIN32_H
#define WINDOW_MANAGER_WIN32_H

#include "window_manager.h"
#include <windows.h>

typedef struct _wm_plat_info {
    HWND      win_handle;
    HINSTANCE mod_handle;
} _wm_plat_info;

internal LRESULT CALLBACK _wm_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

#endif
