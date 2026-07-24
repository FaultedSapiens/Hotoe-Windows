#include <windows.h>

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(
        hwnd,
        uMsg,
        wParam,
        lParam
    );
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    const char CLASS_NAME[] = "HotoeWindow";

    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    DWORD windowStyle = WS_POPUP | WS_CAPTION;

    WNDCLASS windowClass = {0};

    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = CLASS_NAME;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&windowClass))
    {
        MessageBox(
            NULL,
            "RegisterClass failed.",
            "Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Hotoe Windows Prototype",
        windowStyle,

        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,

        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        MessageBox(
            NULL,
            "CreateWindowEx failed!",
            "Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG message = {0};

    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}