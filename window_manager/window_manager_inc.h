#ifndef WINDOW_MANAGER_INC_H
#define WINDOW_MANAGER_INC_H

#include "window_manager.h"

#if PLATFORM_WIN32
#   include "window_manager_win32.h"
#elif PLATFORM_MACOS
#elif PLATFORM_LINUX
#endif

#endif
