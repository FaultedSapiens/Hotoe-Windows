#include <windows.h>
#include <windowsx.h>      // GET_X_LPARAM, GET_Y_LPARAM
#include <dcomp.h>

#include <WebView2.h>
#include <wrl.h>

#include <iostream>
#include <string>
#include <vector>          // std::vector
#include <unordered_map>
#include <algorithm>
#include <cmath>           // floor, ceil, isfinite
#include <iomanip>

using namespace Microsoft::WRL;

// Forward declaration: used by geometry helpers below.
void Log(const char* text);


// ============================================================
// Configuration
// ============================================================

constexpr wchar_t VISUAL_CLASS[] =
   L"HotoeP7AVisual";

constexpr wchar_t INPUT_CLASS[] =
    L"HotoeP7AInput";

constexpr int SIR_X = 100;
constexpr int SIR_Y = 100;
constexpr int SIR_W = 400;
constexpr int SIR_H = 250;


// ============================================================
// HWNDs / P6F1 dynamic native topology
// ============================================================

HWND gVisualWindow = nullptr;
HINSTANCE gInstance = nullptr;

struct InputPiece
{
    HWND hwnd = nullptr;
    bool trackingMouse = false;
    int sirId = -1;
    int pieceId = -1;
    long long generation = -1;
};

std::vector<InputPiece> gInputPieces;

int InputIndex(HWND hwnd)
{
    for (size_t i = 0; i < gInputPieces.size(); ++i)
        if (gInputPieces[i].hwnd == hwnd)
            return static_cast<int>(i);

    return -1;
}

HWND EnsureInputPiece(int pieceIndex)
{
    if (pieceIndex < 0)
        return nullptr;

    while (static_cast<int>(gInputPieces.size()) <= pieceIndex)
    {
        InputPiece piece{};

        piece.hwnd =
            CreateWindowExW(
                WS_EX_NOACTIVATE |
                WS_EX_TOOLWINDOW |
                WS_EX_TOPMOST,
                INPUT_CLASS,
                L"Hotoe P7A Input Piece",
                WS_POPUP,
                0, 0, 1, 1,
                nullptr,
                nullptr,
                gInstance,
                nullptr
            );

        if (!piece.hwnd)
        {
            Log("P7A Create dynamic input HWND FAILED");
            return nullptr;
        }

        ShowWindow(piece.hwnd, SW_HIDE);
        gInputPieces.push_back(piece);
    }

    return gInputPieces[pieceIndex].hwnd;
}

void HideAllInputPieces()
{
    for (auto& piece : gInputPieces)
    {
        if (piece.hwnd)
            ShowWindow(piece.hwnd, SW_HIDE);

        piece.sirId = -1;
        piece.pieceId = -1;
    }
}

// ============================================================
// D3 geometry model
// ============================================================

struct CssRect
{
    double left;
    double top;
    double right;
    double bottom;
};

struct NativeRect
{
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};


// ============================================================
// D4.2 native monotonic clock
// ============================================================
//
// QueryPerformanceCounter defines the native timing domain.
// Browser performance.now() is deliberately treated as a
// separate clock domain; D4.2 records both but does not perform
// an invalid cross-domain subtraction.
//
double NativeNowMs()
{
    static LARGE_INTEGER frequency = []()
    {
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        return f;
    }();

    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);

    return
        1000.0 *
        static_cast<double>(counter.QuadPart) /
        static_cast<double>(frequency.QuadPart);
}


/*
    Coordinate transform:

        T : R^2_css -> R^2_input

    D3 hypothesis H0:

        T = I

    We deliberately do NOT multiply by devicePixelRatio.

    D3 will experimentally determine whether CSS coordinates
    already coincide with the coordinate space used by
    SetWindowPos for our input HWND.
*/
POINTF TransformCssPoint(
    double x,
    double y
)
{
    POINTF p{};

    p.x = static_cast<FLOAT>(x);
    p.y = static_cast<FLOAT>(y);

    return p;
}


/*
    Q : continuous transformed rectangle
        -> discrete Win32 rectangle

    Outward quantization:

        L_native = floor(L)
        T_native = floor(T)
        R_native = ceil(R)
        B_native = ceil(B)

    Therefore:

        Q(T(S_css)) contains T(S_css)

    rather than accidentally shrinking the interactive set.
*/
NativeRect CssToNativeRect(
    const CssRect& css
)
{
    POINTF p0 =
        TransformCssPoint(
            css.left,
            css.top
        );

    POINTF p1 =
        TransformCssPoint(
            css.right,
            css.bottom
        );

    NativeRect native{};

    native.left =
        static_cast<LONG>(
            std::floor(p0.x)
        );

    native.top =
        static_cast<LONG>(
            std::floor(p0.y)
        );

    native.right =
        static_cast<LONG>(
            std::ceil(p1.x)
        );

    native.bottom =
        static_cast<LONG>(
            std::ceil(p1.y)
        );

    return native;
}


bool ApplyNativeInputPiece(
    int slot,
    int sirId,
    int pieceId,
    long long generation,
    const NativeRect& region,
    bool active = true
)
{
    HWND hwnd = EnsureInputPiece(slot);

    if (!hwnd)
        return false;

    InputPiece& piece = gInputPieces[slot];

    piece.sirId = sirId;
    piece.pieceId = pieceId;
    piece.generation = generation;

    if (!active)
    {
        ShowWindow(hwnd, SW_HIDE);
        return true;
    }

    const LONG width = region.right - region.left;
    const LONG height = region.bottom - region.top;

    if (width <= 0 || height <= 0)
    {
        ShowWindow(hwnd, SW_HIDE);
        return false;
    }

    return
        SetWindowPos(
            hwnd,
            HWND_TOPMOST,
            region.left,
            region.top,
            width,
            height,
            SWP_NOACTIVATE | SWP_SHOWWINDOW
        ) != FALSE;
}


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
    HWND inputHwnd,
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
        inputHwnd,
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
    HWND inputHwnd,
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
            inputHwnd,
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
    const int inputIndex = InputIndex(hwnd);

    switch (msg)
    {
        case WM_MOUSEMOVE:
{
    if (inputIndex >= 0 && !gInputPieces[inputIndex].trackingMouse)
    {
        TRACKMOUSEEVENT tme{};

        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;

        if (TrackMouseEvent(&tme))
        {
            gInputPieces[inputIndex].trackingMouse = true;
            Log("TRACE  p(t) entered S_native   :: membership 0 -> 1");
        }
        else
        {
            Log("TrackMouseEvent FAILED");
        }
    }

    SendMouseToWebView(
        hwnd,
        msg,
        wParam,
        lParam
    );

    return 0;
}


case WM_MOUSELEAVE:
{
    if (inputIndex >= 0) gInputPieces[inputIndex].trackingMouse = false;

    Log("TRACE  p(t) left S_native      :: membership 1 -> 0");

    return 0;
}

        case WM_LBUTTONDOWN:
        {
            Log(
                "TRACE  MOUSE_1 : DOWN   | p(t) in S_native"
            );


            SetCapture(
                hwnd
            );


            SendMouseToWebView(
                hwnd,
                msg,
                wParam,
                lParam
            );


            return 0;
        }


        case WM_LBUTTONUP:
        {
            Log(
                "TRACE  MOUSE_1 : UP     | p(t) in S_native"
            );


            SendMouseToWebView(
                hwnd,
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
                hwnd,
                msg,
                wParam,
                lParam
            );


            return 0;
        }


        case WM_MOUSEACTIVATE:
        {
            /*
                P7A remains pointer-only.

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
        This is the critical P7A experiment:

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

            L"Hotoe P7A Visual",

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
        P7A click-through mechanism.

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
    gInstance = instance;

    /*
        P7A no longer allocates exactly four HWNDs.

        Native topology is materialized lazily from the current
        rectangular decomposition of the DOM interaction set.
    */

    gInputPieces.clear();

    Log("P7A native input-surface pool ready");
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

// ============================================================
// D3 minimal geometry-message parser
// ============================================================

bool ExtractJsonNumber(
    const std::wstring& json,
    const std::wstring& key,
    double& value
)
{
    const std::wstring token =
        L"\"" + key + L"\"";

    size_t position =
        json.find(token);

    if (position == std::wstring::npos)
        return false;

    position =
        json.find(
            L':',
            position + token.length()
        );

    if (position == std::wstring::npos)
        return false;

    ++position;

    while (
        position < json.length() &&
        (
            json[position] == L' ' ||
            json[position] == L'\t' ||
            json[position] == L'\r' ||
            json[position] == L'\n'
        )
    )
    {
        ++position;
    }

    try
    {
        size_t consumed = 0;

        value =
            std::stod(
                json.substr(position),
                &consumed
            );

        return consumed > 0;
    }
    catch (...)
    {
        return false;
    }
}


bool ParseInteractiveRegion(
    const std::wstring& json,
    CssRect& region
)
{
    /*
        First make sure this is the message class D3 understands.
    */

    if (
        (json.find(L"\"type\":\"interactive-region\"") == std::wstring::npos &&
         json.find(L"\"type\":\"region-piece\"") == std::wstring::npos)
    )
    {
        return false;
    }

    double left;
    double top;
    double right;
    double bottom;

    if (
        !ExtractJsonNumber(
            json,
            L"left",
            left
        ) ||

        !ExtractJsonNumber(
            json,
            L"top",
            top
        ) ||

        !ExtractJsonNumber(
            json,
            L"right",
            right
        ) ||

        !ExtractJsonNumber(
            json,
            L"bottom",
            bottom
        )
    )
    {
        return false;
    }

    if (
        !std::isfinite(left) ||
        !std::isfinite(top) ||
        !std::isfinite(right) ||
        !std::isfinite(bottom)
    )
    {
        return false;
    }

    if (
        right <= left ||
        bottom <= top
    )
    {
        return false;
    }

    region.left   = left;
    region.top    = top;
    region.right  = right;
    region.bottom = bottom;

    return true;
}


bool ParseInteractiveRegionSnapshot(
    const std::wstring& regionJson,
    CssRect& region
)
{
    double left   = 0.0;
    double top    = 0.0;
    double right  = 0.0;
    double bottom = 0.0;

    if (
        !ExtractJsonNumber(regionJson, L"left", left) ||
        !ExtractJsonNumber(regionJson, L"top", top) ||
        !ExtractJsonNumber(regionJson, L"right", right) ||
        !ExtractJsonNumber(regionJson, L"bottom", bottom)
    )
    {
        return false;
    }

    if (
        !std::isfinite(left) ||
        !std::isfinite(top) ||
        !std::isfinite(right) ||
        !std::isfinite(bottom) ||
        right <= left ||
        bottom <= top
    )
    {
        return false;
    }

    region.left   = left;
    region.top    = top;
    region.right  = right;
    region.bottom = bottom;

    return true;
}

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




                                    EventRegistrationToken webMessageToken{};

hr = gWebView->add_WebMessageReceived(
    Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [](
            ICoreWebView2* sender,
            ICoreWebView2WebMessageReceivedEventArgs* args
        ) -> HRESULT
        {
            LPWSTR rawJson = nullptr;

            HRESULT hr =
                args->get_WebMessageAsJson(&rawJson);

            if (FAILED(hr) || !rawJson)
                return S_OK;

            std::wstring json(rawJson);

            std::wcout
                << L"RX <-- "
                << json
                << std::endl;

            //
            // ping
            //
            if (json.find(L"\"type\":\"ping\"") != std::wstring::npos)
            {
                const wchar_t* reply =
                    LR"({"type":"pong"})";

                sender->PostWebMessageAsJson(reply);

                std::wcout
                    << L"TX --> "
                    << reply
                    << std::endl;
            }

            //
            // echo
            //
            else if (json.find(L"\"type\":\"echo\"") != std::wstring::npos)
            {
                sender->PostWebMessageAsJson(rawJson);

                std::wcout
                    << L"TX --> "
                    << rawJson
                    << std::endl;
            }

            //
            // unknown
            //
            else
            {
                std::wcout
                    << L"Unknown transport message"
                    << std::endl;
            }

            CoTaskMemFree(rawJson);

            return S_OK;
        }
    ).Get(),
    &webMessageToken
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
                                        gWebView->Navigate(
                                        L"file:///C:/Users/acer/Projects/Hotoe-Windows/prototypes/07-ipc/index.html"
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
    "Hotoe Prototype 07A"
);

Log(
    "Bidirectional JSON Transport"
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
        P7A input surface.
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
    "P7A initialization requested"
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