#ifndef __COMMON_H
#define __COMMON_H

#define UNICODE
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>

#define STR_SNAKE               L"Snake"
#define STR_FAILED_START        L"Unable to start program!"
#define STR_ERROR               L"Error"
#define STR_MENU_PAUSE          L"Pa&use"
#define STR_MENU_RESUME         L"&Resume"
#define STR_GAME_OVER           L"You lost! Score: %d"

void CenterWindow(const HWND);
void ResizeCenteredWindow(const HWND, const UINT, const UINT);

#endif
