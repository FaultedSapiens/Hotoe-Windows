#pragma once

#include <windows.h>

extern HWND gWindow;

bool RegisterHotoeWindowClass(
    HINSTANCE hInstance
);

HWND CreateHotoeWindow(
    HINSTANCE hInstance
);

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);