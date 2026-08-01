#pragma once

#include <windows.h>

bool InitializeDirectComposition(HWND hwnd);

HRESULT InitializeWebView2();

void ResizeWebView(HWND hwnd);

bool ForwardMouseToWebView(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

void SendWebViewMouseLeave(POINT point);

void FocusWebView();