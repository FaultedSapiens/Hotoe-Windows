#include <windows.h>
#include <windowsx.h>
#include <dcomp.h>

#include <WebView2.h>
#include <wrl.h>

#include <cstdio>
#include <iostream>

using namespace Microsoft::WRL;


// ============================================================
// Configuration
// ============================================================

constexpr wchar_t VISUAL_CLASS[] =
    L"HotoeP6CVisual";

constexpr wchar_t INPUT_CLASS[] =
    L"HotoeP6CInput";

constexpr int SIR_X = 100;
constexpr int SIR_Y = 100;
constexpr int SIR_W = 400;
constexpr int SIR_H = 250;


// ============================================================
// HWNDs
// ============================================================

HWND gVisualWindow = nullptr;
HWND gInputWindow  = nullptr;

// Tracks whether TrackMouseEvent(TME_LEAVE) is currently armed.
bool gTrackingInputMouse = false;


// ============================================================
// DirectComposition
// ============================================================

ComPtr<IDCompositionDevice>
    gDCompDevice;

ComPtr<IDCompositionTarget>
    gDCompTarget;

ComPtr<IDCompositionVisual>
    gRootVisual;

ComPtr<IDCompositionVisual>
    gWebViewVisual;


// ============================================================
// WebView2
// ============================================================

ComPtr<ICoreWebView2CompositionController>
    gCompositionController;

ComPtr<ICoreWebView2Controller>
    gWebViewController;

ComPtr<ICoreWebView2>
    gWebView;


// ============================================================
// Logging
// ============================================================

void Log(const char* text)
{
    std::cout
        << text
        << '\n';
}


void LogHR(
    const char* operation,
    HRESULT hr
)
{
    std::cout
        << operation
        << " HRESULT=0x"
        << std::hex
        << static_cast<unsigned long>(hr)
        << std::dec
        << '\n';
}


// ============================================================
// Mouse conversion
// ============================================================

COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS
GetMouseKeys(WPARAM wParam)
{
    UINT32 keys =
        COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE;

    if (wParam & MK_LBUTTON)
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_LEFT_BUTTON;

    if (wParam & MK_RBUTTON)
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_RIGHT_BUTTON;

    if (wParam & MK_MBUTTON)
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_MIDDLE_BUTTON;

    if (wParam & MK_SHIFT)
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_SHIFT;

    if (wParam & MK_CONTROL)
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_CONTROL;

    return static_cast<
        COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS
    >(keys);
}


// ============================================================
// INPUT HWND -> WEBVIEW coordinate conversion
// ============================================================

POINT InputPointToWebView(
    LPARAM lParam
)
{
    /*
        lParam is relative to the small input HWND.

        Convert:

            input client
                 ↓
            screen
                 ↓
            visual client

        The WebView occupies the fullscreen visual HWND,
        therefore visual-client coordinates are WebView
        coordinates.
    */

    POINT point{
        GET_X_LPARAM(lParam),
        GET_Y_LPARAM(lParam)
    };

    ClientToScreen(
        gInputWindow,
        &point
    );

    ScreenToClient(
        gVisualWindow,
        &point
    );

    return point;
}


// ============================================================
// Send input into WebView2
// ============================================================

void SendMouseToWebView(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    if (!gCompositionController)
        return;


    COREWEBVIEW2_MOUSE_EVENT_KIND kind;


    switch (message)
    {
        case WM_MOUSEMOVE:

            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_MOVE;

            break;


        case WM_LBUTTONDOWN:

            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOWN;

            break;


        case WM_LBUTTONUP:

            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP;

            break;


        case WM_RBUTTONDOWN:

            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOWN;

            break;


        case WM_RBUTTONUP:

            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP;

            break;


        case WM_MBUTTONDOWN:

            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_DOWN;

            break;


        case WM_MBUTTONUP:

            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_UP;

            break;


        default:
            return;
    }


    POINT point =
        InputPointToWebView(
            lParam
        );


    HRESULT hr =
        gCompositionController
            ->SendMouseInput(
                kind,
                GetMouseKeys(wParam),
                0,
                point
            );


    if (FAILED(hr))
    {
        LogHR(
            "SendMouseInput FAILED",
            hr
        );
    }
}


// ============================================================
// Visual HWND
// ============================================================

LRESULT CALLBACK VisualProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (msg)
    {
        case WM_MOUSEACTIVATE:

            return MA_NOACTIVATE;


        case WM_ERASEBKGND:

            return 1;


        case WM_DESTROY:

            return 0;
    }


    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}


// ============================================================
// Input HWND
// ============================================================

LRESULT CALLBACK InputProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (msg)
    {
        case WM_MOUSEMOVE:
{
    if (!gTrackingInputMouse)
    {
        TRACKMOUSEEVENT tme{};

        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;

        if (TrackMouseEvent(&tme))
        {
            gTrackingInputMouse = true;
            Log("Pointer entered SIR");
        }
        else
        {
            Log("TrackMouseEvent FAILED");
        }
    }

    SendMouseToWebView(
        msg,
        wParam,
        lParam
    );

    return 0;
}


case WM_MOUSELEAVE:
{
    gTrackingInputMouse = false;

    Log("Pointer left SIR");

    return 0;
}

        case WM_LBUTTONDOWN:
        {
            Log(
                "SIR -> WM_LBUTTONDOWN"
            );


            SetCapture(
                hwnd
            );


            SendMouseToWebView(
                msg,
                wParam,
                lParam
            );


            return 0;
        }


        case WM_LBUTTONUP:
        {
            Log(
                "SIR -> WM_LBUTTONUP"
            );


            SendMouseToWebView(
                msg,
                wParam,
                lParam
            );


            if (
                GetCapture() ==
                hwnd
            )
            {
                ReleaseCapture();
            }


            return 0;
        }


        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        {
            SendMouseToWebView(
                msg,
                wParam,
                lParam
            );


            return 0;
        }


        case WM_MOUSEACTIVATE:
        {
            /*
                P6C remains pointer-only.

                Focus/keyboard comes later.
            */

            return MA_NOACTIVATE;
        }


        case WM_ERASEBKGND:

            return 1;


        case WM_DESTROY:

            return 0;
    }


    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}


// ============================================================
// Window classes
// ============================================================

bool RegisterClasses(
    HINSTANCE instance
)
{
    WNDCLASSW visual{};

    visual.lpfnWndProc =
        VisualProc;

    visual.hInstance =
        instance;

    visual.lpszClassName =
        VISUAL_CLASS;

    visual.hCursor =
        LoadCursorW(
            nullptr,
            IDC_ARROW
        );

    visual.hbrBackground =
        nullptr;


    if (!RegisterClassW(
            &visual
        ))
    {
        Log(
            "Visual RegisterClass FAILED"
        );

        return false;
    }


    WNDCLASSW input{};

    input.lpfnWndProc =
        InputProc;

    input.hInstance =
        instance;

    input.lpszClassName =
        INPUT_CLASS;

    input.hCursor =
        LoadCursorW(
            nullptr,
            IDC_HAND
        );

    input.hbrBackground =
        nullptr;


    if (!RegisterClassW(
            &input
        ))
    {
        Log(
            "Input RegisterClass FAILED"
        );

        return false;
    }


    return true;
}


// ============================================================
// Visual surface
// ============================================================

bool CreateVisualWindow(
    HINSTANCE instance
)
{
    const int width =
        GetSystemMetrics(
            SM_CXSCREEN
        );

    const int height =
        GetSystemMetrics(
            SM_CYSCREEN
        );


    /*
        This is the critical P6C experiment:

        DirectComposition target
                    +
        layered / transparent HWND
    */

    gVisualWindow =
        CreateWindowExW(
            WS_EX_LAYERED |
            WS_EX_TRANSPARENT |
            WS_EX_NOACTIVATE |
            WS_EX_TOOLWINDOW |
            WS_EX_TOPMOST,

            VISUAL_CLASS,

            L"Hotoe P6C Visual",

            WS_POPUP,

            0,
            0,
            width,
            height,

            nullptr,
            nullptr,
            instance,
            nullptr
        );


    if (!gVisualWindow)
    {
        Log(
            "Create visual HWND FAILED"
        );

        return false;
    }


    /*
        P6A click-through mechanism.

        Black is the color key.

        DirectComposition/WebView content is then
        composed onto this HWND.
    */

    if (!SetLayeredWindowAttributes(
            gVisualWindow,
            RGB(0, 0, 0),
            255,
            LWA_COLORKEY
        ))
    {
        Log(
            "SetLayeredWindowAttributes FAILED"
        );

        return false;
    }


    ShowWindow(
        gVisualWindow,
        SW_SHOWNOACTIVATE
    );


    Log(
        "Visual HWND created"
    );


    return true;
}


// ============================================================
// Input surface
// ============================================================

bool CreateInputWindow(
    HINSTANCE instance
)
{
    gInputWindow =
        CreateWindowExW(
            WS_EX_NOACTIVATE |
            WS_EX_TOOLWINDOW |
            WS_EX_TOPMOST,

            INPUT_CLASS,

            L"Hotoe P6C Input",

            WS_POPUP,

            SIR_X,
            SIR_Y,
            SIR_W,
            SIR_H,

            nullptr,
            nullptr,
            instance,
            nullptr
        );


    if (!gInputWindow)
    {
        Log(
            "Create input HWND FAILED"
        );

        return false;
    }


    ShowWindow(
        gInputWindow,
        SW_SHOWNOACTIVATE
    );


    SetWindowPos(
        gInputWindow,
        HWND_TOPMOST,

        SIR_X,
        SIR_Y,
        SIR_W,
        SIR_H,

        SWP_NOACTIVATE |
        SWP_SHOWWINDOW
    );


    Log(
        "Input HWND created"
    );


    return true;
}


// ============================================================
// DirectComposition
// ============================================================

bool InitializeComposition()
{
    HRESULT hr =
        DCompositionCreateDevice(
            nullptr,

            __uuidof(
                IDCompositionDevice
            ),

            reinterpret_cast<void**>(
                gDCompDevice
                    .GetAddressOf()
            )
        );


    if (FAILED(hr))
    {
        LogHR(
            "DCompositionCreateDevice",
            hr
        );

        return false;
    }


    Log(
        "DComp device created"
    );


    hr =
        gDCompDevice
            ->CreateTargetForHwnd(
                gVisualWindow,
                TRUE,
                &gDCompTarget
            );


    if (FAILED(hr))
    {
        LogHR(
            "CreateTargetForHwnd",
            hr
        );

        return false;
    }


    /*
        Microsoft explicitly permits the composition
        target HWND to be a layered window.
    */


    hr =
        gDCompDevice
            ->CreateVisual(
                &gRootVisual
            );


    if (FAILED(hr))
    {
        LogHR(
            "Create root visual",
            hr
        );

        return false;
    }


    hr =
        gDCompDevice
            ->CreateVisual(
                &gWebViewVisual
            );


    if (FAILED(hr))
    {
        LogHR(
            "Create WebView visual",
            hr
        );

        return false;
    }


    hr =
        gRootVisual
            ->AddVisual(
                gWebViewVisual.Get(),
                TRUE,
                nullptr
            );


    if (FAILED(hr))
    {
        LogHR(
            "AddVisual",
            hr
        );

        return false;
    }


    hr =
        gDCompTarget
            ->SetRoot(
                gRootVisual.Get()
            );


    if (FAILED(hr))
    {
        LogHR(
            "SetRoot",
            hr
        );

        return false;
    }


    hr =
        gDCompDevice
            ->Commit();


    if (FAILED(hr))
    {
        LogHR(
            "Initial Commit",
            hr
        );

        return false;
    }


    Log(
        "DirectComposition ready"
    );


    return true;
}


// ============================================================
// WebView2
// ============================================================

HRESULT InitializeWebView()
{
    return
        CreateCoreWebView2EnvironmentWithOptions(
            nullptr,
            nullptr,
            nullptr,

            Callback<
                ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
            >(
                [](
                    HRESULT result,
                    ICoreWebView2Environment* environment
                ) -> HRESULT
                {
                    if (
                        FAILED(result) ||
                        !environment
                    )
                    {
                        LogHR(
                            "WebView environment",
                            result
                        );

                        return result;
                    }


                    Log(
                        "WebView2 environment created"
                    );


                    ComPtr<
                        ICoreWebView2Environment3
                    > environment3;


                    HRESULT hr =
                        environment
                            ->QueryInterface(
                                IID_PPV_ARGS(
                                    &environment3
                                )
                            );


                    if (
                        FAILED(hr) ||
                        !environment3
                    )
                    {
                        LogHR(
                            "Environment3",
                            hr
                        );

                        return hr;
                    }


                    return environment3
                        ->CreateCoreWebView2CompositionController(
                            gVisualWindow,

                            Callback<
                                ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler
                            >(
                                [](
                                    HRESULT result,
                                    ICoreWebView2CompositionController*
                                        controller
                                ) -> HRESULT
                                {
                                    if (
                                        FAILED(result) ||
                                        !controller
                                    )
                                    {
                                        LogHR(
                                            "CompositionController",
                                            result
                                        );

                                        return result;
                                    }


                                    gCompositionController =
                                        controller;


                                    Log(
                                        "CompositionController created"
                                    );


                                    HRESULT hr =
                                        gCompositionController.As(
                                            &gWebViewController
                                        );


                                    if (FAILED(hr))
                                        return hr;


                                    hr =
                                        gWebViewController
                                            ->get_CoreWebView2(
                                                &gWebView
                                            );


                                    if (FAILED(hr))
                                        return hr;

// ========================================================
// P6D1: JavaScript -> native messaging
// ========================================================

EventRegistrationToken webMessageToken{};

hr = gWebView->add_WebMessageReceived(

    Callback<
        ICoreWebView2WebMessageReceivedEventHandler
    >(
        [](
            ICoreWebView2*,
            ICoreWebView2WebMessageReceivedEventArgs* args
        ) -> HRESULT
        {
            LPWSTR json = nullptr;

            HRESULT messageHr =
                args->get_WebMessageAsJson(
                    &json
                );

            if (FAILED(messageHr))
            {
                LogHR(
                    "get_WebMessageAsJson FAILED",
                    messageHr
                );

                return S_OK;
            }

            std::wcout
                << L"D1 JS -> Native: "
                << json
                << L'\n';

            CoTaskMemFree(json);

            return S_OK;
        }
    ).Get(),

    &webMessageToken
);


if (FAILED(hr))
{
    LogHR(
        "add_WebMessageReceived FAILED",
        hr
    );

    return hr;
}


Log(
    "D1 WebMessageReceived handler installed"
);
                                    // ------------------------
                                    // Transparent WebView
                                    // ------------------------

                                    ComPtr<
                                        ICoreWebView2Controller2
                                    > controller2;


                                    if (
                                        SUCCEEDED(
                                            gWebViewController.As(
                                                &controller2
                                            )
                                        )
                                    )
                                    {
                                        COREWEBVIEW2_COLOR transparent{
                                            0,
                                            0,
                                            0,
                                            0
                                        };


                                        controller2
                                            ->put_DefaultBackgroundColor(
                                                transparent
                                            );
                                    }


                                    // ------------------------
                                    // Full visual bounds
                                    // ------------------------

                                    RECT bounds{};

                                    GetClientRect(
                                        gVisualWindow,
                                        &bounds
                                    );


                                    hr =
                                        gWebViewController
                                            ->put_Bounds(
                                                bounds
                                            );


                                    if (FAILED(hr))
                                        return hr;


                                    // ------------------------
                                    // Connect WebView2 tree
                                    // ------------------------

                                    hr =
                                        gCompositionController
                                            ->put_RootVisualTarget(
                                                gWebViewVisual.Get()
                                            );


                                    if (FAILED(hr))
                                    {
                                        LogHR(
                                            "RootVisualTarget",
                                            hr
                                        );

                                        return hr;
                                    }


                                    hr =
                                        gDCompDevice
                                            ->Commit();


                                    if (FAILED(hr))
                                        return hr;


                                    Log(
                                        "WebView attached to DComp"
                                    );


                                    // ------------------------
                                    // Navigate
                                    // ------------------------

                                    hr =
                                        gWebView
                                            ->Navigate(
                                                L"file:///C:/Users/acer/Projects/"
                                                L"Hotoe-Windows/prototypes/"
                                                L"06d-dynamic-regions/index.html"
                                            );


                                    if (FAILED(hr))
                                    {
                                        LogHR(
                                            "Navigate",
                                            hr
                                        );

                                        return hr;
                                    }


                                    Log(
                                        "Navigation started"
                                    );


                                    return S_OK;
                                }
                            ).Get()
                        );
                }
            ).Get()
        );
}


// ============================================================
// Main
// ============================================================

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int
)
{
    AllocConsole();


    FILE* fp = nullptr;


    freopen_s(
        &fp,
        "CONOUT$",
        "w",
        stdout
    );


    Log(
        "========================================"
    );

    Log(
    "Hotoe Prototype 06D / D1"
);

Log(
    "JS -> Native Region Messaging"
);

    Log(
        "========================================"
    );


    HRESULT com =
        CoInitializeEx(
            nullptr,
            COINIT_APARTMENTTHREADED
        );


    if (FAILED(com))
    {
        LogHR(
            "CoInitializeEx",
            com
        );

        return 1;
    }


    if (!RegisterClasses(instance))
        return 1;


    if (!CreateVisualWindow(instance))
        return 1;


    /*
        Establish composition BEFORE WebView2.
    */

    if (!InitializeComposition())
        return 1;


    /*
        P6B input surface.
    */

    if (!CreateInputWindow(instance))
        return 1;


    HRESULT hr =
        InitializeWebView();


    if (FAILED(hr))
    {
        LogHR(
            "InitializeWebView immediate",
            hr
        );

        return 1;
    }


   Log(
    "P6D1 initialization requested"
);


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
        TranslateMessage(
            &msg
        );

        DispatchMessageW(
            &msg
        );
    }


    CoUninitialize();

    return 0;
}