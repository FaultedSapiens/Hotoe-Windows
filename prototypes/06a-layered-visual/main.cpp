#include <windows.h>

constexpr wchar_t CLASS_NAME[] = L"HotoeP6ALayeredVisual";

// ------------------------------------------------------------
// Window procedure
// ------------------------------------------------------------

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (msg)
    {
        case WM_ERASEBKGND:
            return 1;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

// ------------------------------------------------------------
// Entry
// ------------------------------------------------------------

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int
)
{
    WNDCLASSW wc{};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(
            nullptr,
            L"RegisterClassW failed",
            L"Hotoe P6A",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    const int screenWidth =
        GetSystemMetrics(SM_CXSCREEN);

    const int screenHeight =
        GetSystemMetrics(SM_CYSCREEN);

    // --------------------------------------------------------
    // Visual-only overlay
    //
    // WS_EX_LAYERED:
    //     enables layered-window transparency.
    //
    // WS_EX_TRANSPARENT:
    //     layered window passes mouse input through.
    //
    // WS_EX_NOACTIVATE:
    //     overlay should never steal activation.
    //
    // WS_EX_TOOLWINDOW:
    //     don't behave like a normal application window.
    //
    // WS_EX_TOPMOST:
    //     remain above ordinary windows.
    // --------------------------------------------------------

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED |
        WS_EX_TRANSPARENT |
        WS_EX_NOACTIVATE |
        WS_EX_TOOLWINDOW |
        WS_EX_TOPMOST,

        CLASS_NAME,
        L"Hotoe P6A",

        WS_POPUP,

        0,
        0,
        screenWidth,
        screenHeight,

        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
    {
        MessageBoxW(
            nullptr,
            L"CreateWindowExW failed",
            L"Hotoe P6A",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    // --------------------------------------------------------
    // COLOR KEY
    //
    // Everything painted RGB(0,0,0) becomes transparent.
    //
    // Red stays visible.
    // --------------------------------------------------------

    if (!SetLayeredWindowAttributes(
            hwnd,
            RGB(0, 0, 0),
            255,
            LWA_COLORKEY
        ))
    {
        MessageBoxW(
            nullptr,
            L"SetLayeredWindowAttributes failed",
            L"Hotoe P6A",
            MB_OK | MB_ICONERROR
        );

        DestroyWindow(hwnd);

        return 1;
    }

    ShowWindow(
        hwnd,
        SW_SHOWNOACTIVATE
    );

    UpdateWindow(hwnd);

    // --------------------------------------------------------
    // Paint our experiment directly onto the HWND.
    //
    // BLACK = invisible because it is our color key.
    // RED   = visible.
    // --------------------------------------------------------

    HDC dc = GetDC(hwnd);

    if (!dc)
    {
        DestroyWindow(hwnd);
        return 1;
    }

    RECT entireScreen{
        0,
        0,
        screenWidth,
        screenHeight
    };

    HBRUSH transparentBrush =
        CreateSolidBrush(
            RGB(0, 0, 0)
        );

    FillRect(
        dc,
        &entireScreen,
        transparentBrush
    );

    DeleteObject(
        transparentBrush
    );

    RECT redBox{
        100,
        100,
        500,
        350
    };

    HBRUSH redBrush =
        CreateSolidBrush(
            RGB(255, 0, 0)
        );

    FillRect(
        dc,
        &redBox,
        redBrush
    );

    DeleteObject(
        redBrush
    );

    // Draw some text so we're absolutely sure we're seeing
    // Hotoe rather than something underneath.

    SetBkMode(
        dc,
        TRANSPARENT
    );

    SetTextColor(
        dc,
        RGB(255, 255, 255)
    );

    const wchar_t text[] =
        L"HOTOE P6A - CLICK THROUGH ME";

    TextOutW(
        dc,
        125,
        200,
        text,
        ARRAYSIZE(text) - 1
    );

    ReleaseDC(
        hwnd,
        dc
    );

    // --------------------------------------------------------
    // Message loop
    // --------------------------------------------------------

    MSG msg{};

    while (
        GetMessageW(
            &msg,
            nullptr,
            0,
            0
        ) > 0
    )
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}