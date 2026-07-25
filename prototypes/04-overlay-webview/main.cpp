#include <windows.h>
#include <WebView2.h>

#include <wrl.h>

using namespace Microsoft::WRL;

ComPtr<ICoreWebView2Controller> webviewController;
ComPtr<ICoreWebView2> webview;


LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
{
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

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
    const wchar_t CLASS_NAME[] = L"HotoeWindow";

    

    DWORD windowStyle = WS_POPUP;

    WNDCLASS windowClass = {0};

    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = CLASS_NAME;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&windowClass))
    {
        MessageBox(
            NULL,
            L"RegisterClass failed.",
            L"Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Hotoe Windows Prototype",
        windowStyle,

        0,
        0,
        screenWidth,
        screenHeight,

        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        MessageBox(
            NULL,
            L"CreateWindowEx failed!",
            L"Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    CreateCoreWebView2EnvironmentWithOptions(
    nullptr,
    nullptr,
    nullptr,
    Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
        {
            if (!env)
                return E_FAIL;

            env->CreateCoreWebView2Controller(
                hwnd,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                    {
                        if (!controller)
    return E_FAIL;

webviewController = controller;

RECT bounds;
GetClientRect(hwnd, &bounds);
webviewController->put_Bounds(bounds);

webviewController->get_CoreWebView2(&webview);

webview->NavigateToString(
    LR"(
<!DOCTYPE html>
<html>
<body style="background:#202020;color:white;
display:flex;
justify-content:center;
align-items:center;
height:100vh;
font-family:Segoe UI;
font-size:48px;">
Hello Hotoe
</body>
</html>
)"
);

return S_OK;
                    }
                ).Get()
            );

            return S_OK;
        }
    ).Get()
);

    MSG message = {0};

    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}