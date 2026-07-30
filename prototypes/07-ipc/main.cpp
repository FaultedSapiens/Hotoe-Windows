#include <windows.h>
#include <windowsx.h>
#include <WebView2.h>
#include <wrl.h>

#include <iostream>
#include <cstdio>
#include <deque>
#include <string>

using namespace Microsoft::WRL;

constexpr wchar_t WINDOW_CLASS[] = L"HotoeWindow";
constexpr wchar_t WINDOW_TITLE[] = L"Hotoe Prototype 07 - IPC";
constexpr DWORD WINDOW_STYLE = WS_POPUP;

ComPtr<ICoreWebView2Controller> gWebViewController;
ComPtr<ICoreWebView2> gWebView;

HWND gWindow = nullptr;

struct EngineState
{
    // Window
    bool active = false;
    bool focused = false;
    bool foreground = false;
    bool minimized = false;
    bool maximized = false;

    int width = 0;
    int height = 0;

    // Mouse
    int mouseX = 0;
    int mouseY = 0;

    bool mouseInside = false;

    bool leftDown = false;
    bool middleDown = false;
    bool rightDown = false;

    // Keyboard
    UINT lastKey = 0;

    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    // Statistics
    uint64_t mouseMoves = 0;
    uint64_t clicks = 0;
    uint64_t keyDowns = 0;
    uint64_t activates = 0;
    uint64_t focusChanges = 0;
};

EngineState gState;

struct InspectorEvent
{
    SYSTEMTIME time;
    std::string text;
};

std::deque<InspectorEvent> gEventLog;

constexpr size_t MAX_LOG_ENTRIES = 100;

void LogEvent(const std::string& text)
{
    std::cout << text << '\n';

    InspectorEvent event{};

    GetLocalTime(&event.time);

    event.text = text;

    gEventLog.push_front(event);

    if (gEventLog.size() > MAX_LOG_ENTRIES)
        gEventLog.pop_back();
}

void RefreshInspector()
{
    // Serialize EngineState to JSON and send it to the WebView.
    // Prototype 06 will implement this.
}
void UpdateModifierKeys()
{
    gState.ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    gState.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    gState.alt   = (GetKeyState(VK_MENU) & 0x8000) != 0;
}

void ResizeWebView(HWND hwnd)
{
    if (!gWebViewController)
        return;

    RECT bounds;
    GetClientRect(hwnd, &bounds);

    gState.width  = bounds.right - bounds.left;
    gState.height = bounds.bottom - bounds.top;

    gWebViewController->put_Bounds(bounds);
}

void Notify(const std::string& event)
{
    LogEvent(event);
    RefreshInspector();
}

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_SETFOCUS:
{
    gState.focused = true;
    gState.focusChanges++;

    Notify("WM_SETFOCUS");
    break;
}

    case WM_KILLFOCUS:
        gState.focused = false;
        gState.focusChanges++;
        Notify("WM_KILLFOCUS");
        break;

   case WM_MOUSEMOVE:
{
    gState.mouseX = GET_X_LPARAM(lParam);
    gState.mouseY = GET_Y_LPARAM(lParam);

    gState.mouseMoves++;

    if (!gState.mouseInside)
    {
        gState.mouseInside = true;

        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;

        TrackMouseEvent(&tme);
    }

    static DWORD lastUpdate = 0;

    DWORD now = GetTickCount();

    if (now - lastUpdate >= 16)
    {
        lastUpdate = now;
        RefreshInspector();
    }

    break;
}

case WM_MOUSELEAVE:
{
    gState.mouseInside = false;

    Notify("WM_MOUSELEAVE");

    break;
}

case WM_KEYUP:
{
    UpdateModifierKeys();

    Notify("WM_KEYUP");

    break;

}
    case WM_LBUTTONDOWN:
{
    gState.leftDown = true;
    gState.clicks++;

    Notify("WM_LBUTTONDOWN");
    break;
}
case WM_LBUTTONUP:
{
    gState.leftDown = false;

    Notify("WM_LBUTTONUP");



    break;
}

    case WM_MOUSEACTIVATE:
        LogEvent("WM_MOUSEACTIVATE");
        break;

    case WM_ACTIVATE:
{
    WORD state = LOWORD(wParam);

    gState.active = (state != WA_INACTIVE);
    gState.foreground = (GetForegroundWindow() == hwnd);

    if (gState.active)
    {
        gState.activates++;
    }

    if (state == WA_ACTIVE)
    {
        LogEvent("WM_ACTIVATE (ACTIVE)");
    }
    else if (state == WA_CLICKACTIVE)
    {
        LogEvent("WM_ACTIVATE (CLICKACTIVE)");
    }
    else
    {
        LogEvent("WM_ACTIVATE (INACTIVE)");
    }

    RefreshInspector();
    break;

}

    case WM_ACTIVATEAPP:
{
    LogEvent(
        std::string("WM_ACTIVATEAPP: ")
        + (wParam ? "TRUE" : "FALSE")
    );

    break;
}

    case WM_KEYDOWN:
{
    gState.lastKey = static_cast<UINT>(wParam);
    gState.keyDowns++;

     UpdateModifierKeys();

    Notify("WM_KEYDOWN");

    if (wParam == VK_ESCAPE)
    {
        DestroyWindow(hwnd);
        return 0;
    }

    break;

}
case WM_SIZE:
{
    ResizeWebView(hwnd);

    gState.minimized = (wParam == SIZE_MINIMIZED);
    gState.maximized = (wParam == SIZE_MAXIMIZED);

    Notify("WM_SIZE");

    break;
}
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    LogEvent("=== Prototype 07 Started ===");

    

    WNDCLASS windowClass = {0};

    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
   windowClass.lpszClassName = WINDOW_CLASS; 
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
        gWindow = CreateWindowEx(
    0,
    WINDOW_CLASS,
    WINDOW_TITLE,
        WINDOW_STYLE,

        0,
        0,
        screenWidth,
        screenHeight,

        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (gWindow == NULL)
    {
        MessageBox(
            NULL,
            L"CreateWindowEx failed!",
            L"Error",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    ShowWindow(gWindow, SW_SHOW);
    UpdateWindow(gWindow);
    ResizeWebView(gWindow);
    CreateCoreWebView2EnvironmentWithOptions(
    nullptr,
    nullptr,
    nullptr,
    Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
    [](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
        {
            if (FAILED(result) || !env)
    return E_FAIL;

            env->CreateCoreWebView2Controller(
                gWindow,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
                    {
                        if (FAILED(result) || !controller)
    return E_FAIL;

gWebViewController = controller;

RECT bounds;
GetClientRect(gWindow, &bounds);
gWebViewController->put_Bounds(bounds);

HRESULT hr = gWebViewController->get_CoreWebView2(&gWebView);

if (FAILED(hr))
{
    return hr;
}

hr = gWebView->Navigate(
    L"file:///C:/Users/acer/Projects/Hotoe-Windows/prototypes/06-click-through/index.html"
);

if (FAILED(hr))
{
    return hr;
}


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

