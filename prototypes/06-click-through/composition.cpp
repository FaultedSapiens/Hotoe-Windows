#include "composition.h"
#include "state.h"

#include <windowsx.h>
#include <dcomp.h>
#include <WebView2.h>
#include <wrl.h>

#include <iostream>

using namespace Microsoft::WRL;


// ------------------------------------------------------------
// Visual window supplied by main.cpp
// ------------------------------------------------------------

extern HWND gWindow;


// ------------------------------------------------------------
// DirectComposition
// ------------------------------------------------------------

ComPtr<IDCompositionDevice> gDCompDevice;
ComPtr<IDCompositionTarget> gDCompTarget;
ComPtr<IDCompositionVisual> gRootVisual;
ComPtr<IDCompositionVisual> gWebViewVisual;


// ------------------------------------------------------------
// WebView2
// ------------------------------------------------------------

ComPtr<ICoreWebView2CompositionController>
    gCompositionController;

ComPtr<ICoreWebView2Controller>
    gWebViewController;

ComPtr<ICoreWebView2>
    gWebView;


// ------------------------------------------------------------
// HRESULT logging
// ------------------------------------------------------------

void LogHRESULT(
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


// ------------------------------------------------------------
// Mouse key conversion
// ------------------------------------------------------------

COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS
GetWebViewMouseKeys(
    WPARAM wParam
)
{
    UINT32 keys =
        COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE;

    if (wParam & MK_LBUTTON)
    {
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_LEFT_BUTTON;
    }

    if (wParam & MK_RBUTTON)
    {
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_RIGHT_BUTTON;
    }

    if (wParam & MK_MBUTTON)
    {
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_MIDDLE_BUTTON;
    }

    if (wParam & MK_SHIFT)
    {
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_SHIFT;
    }

    if (wParam & MK_CONTROL)
    {
        keys |=
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_CONTROL;
    }

    return static_cast<
        COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS
    >(keys);
}


// ------------------------------------------------------------
// Forward Win32 mouse -> WebView2
// ------------------------------------------------------------

bool ForwardMouseToWebView(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    if (!gCompositionController)
    {
        return false;
    }

    COREWEBVIEW2_MOUSE_EVENT_KIND kind;

    UINT32 mouseData = 0;

    POINT point{
        GET_X_LPARAM(lParam),
        GET_Y_LPARAM(lParam)
    };

    switch (message)
    {
        case WM_MOUSEMOVE:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_MOVE;

            break;
        }

        case WM_LBUTTONDOWN:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_DOWN;

            SetCapture(gWindow);

            break;
        }

        case WM_LBUTTONUP:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_LEFT_BUTTON_UP;

            ReleaseCapture();

            break;
        }

        case WM_RBUTTONDOWN:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_DOWN;

            SetCapture(gWindow);

            break;
        }

        case WM_RBUTTONUP:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_RIGHT_BUTTON_UP;

            ReleaseCapture();

            break;
        }

        case WM_MBUTTONDOWN:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_DOWN;

            SetCapture(gWindow);

            break;
        }

        case WM_MBUTTONUP:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_MIDDLE_BUTTON_UP;

            ReleaseCapture();

            break;
        }

        case WM_MOUSEWHEEL:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_WHEEL;

            mouseData =
                static_cast<UINT32>(
                    GET_WHEEL_DELTA_WPARAM(
                        wParam
                    )
                );

            // Wheel coordinates are screen coordinates.
            point.x =
                GET_X_LPARAM(lParam);

            point.y =
                GET_Y_LPARAM(lParam);

            ScreenToClient(
                gWindow,
                &point
            );

            break;
        }

        case WM_MOUSEHWHEEL:
        {
            kind =
                COREWEBVIEW2_MOUSE_EVENT_KIND_HORIZONTAL_WHEEL;

            mouseData =
                static_cast<UINT32>(
                    GET_WHEEL_DELTA_WPARAM(
                        wParam
                    )
                );

            point.x =
                GET_X_LPARAM(lParam);

            point.y =
                GET_Y_LPARAM(lParam);

            ScreenToClient(
                gWindow,
                &point
            );

            break;
        }

        default:
        {
            return false;
        }
    }

    HRESULT hr =
        gCompositionController->SendMouseInput(
            kind,
            GetWebViewMouseKeys(wParam),
            mouseData,
            point
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "SendMouseInput failed",
            hr
        );

        return false;
    }

    return true;
}


// ------------------------------------------------------------
// Mouse leave
// ------------------------------------------------------------

void SendWebViewMouseLeave(
    POINT point
)
{
    if (!gCompositionController)
    {
        return;
    }

    HRESULT hr =
        gCompositionController->SendMouseInput(
            COREWEBVIEW2_MOUSE_EVENT_KIND_LEAVE,
            COREWEBVIEW2_MOUSE_EVENT_VIRTUAL_KEYS_NONE,
            0,
            point
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "SendMouseInput LEAVE failed",
            hr
        );
    }
}


// ------------------------------------------------------------
// WebView focus
// ------------------------------------------------------------

void FocusWebView()
{
    if (!gWebViewController)
    {
        return;
    }

    HRESULT hr =
        gWebViewController->MoveFocus(
            COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "MoveFocus failed",
            hr
        );
    }
}


// ------------------------------------------------------------
// DirectComposition
// ------------------------------------------------------------

bool InitializeDirectComposition(
    HWND hwnd
)
{
    HRESULT hr =
        DCompositionCreateDevice(
            nullptr,
            __uuidof(IDCompositionDevice),
            reinterpret_cast<void**>(
                gDCompDevice.GetAddressOf()
            )
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "DCompositionCreateDevice failed",
            hr
        );

        return false;
    }

    LogEvent(
        "DirectComposition device created"
    );

    hr =
        gDCompDevice->CreateTargetForHwnd(
            hwnd,
            TRUE,
            &gDCompTarget
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "CreateTargetForHwnd failed",
            hr
        );

        return false;
    }

    LogEvent(
        "DirectComposition target created"
    );

    hr =
        gDCompDevice->CreateVisual(
            &gRootVisual
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "Create root visual failed",
            hr
        );

        return false;
    }

    hr =
        gDCompDevice->CreateVisual(
            &gWebViewVisual
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "Create WebView visual failed",
            hr
        );

        return false;
    }

    hr =
        gRootVisual->AddVisual(
            gWebViewVisual.Get(),
            TRUE,
            nullptr
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "AddVisual failed",
            hr
        );

        return false;
    }

    hr =
        gDCompTarget->SetRoot(
            gRootVisual.Get()
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "SetRoot failed",
            hr
        );

        return false;
    }

    hr =
        gDCompDevice->Commit();

    if (FAILED(hr))
    {
        LogHRESULT(
            "Initial DComp Commit failed",
            hr
        );

        return false;
    }

    LogEvent(
        "DirectComposition initialized"
    );

    return true;
}


// ------------------------------------------------------------
// Resize
// ------------------------------------------------------------

void ResizeWebView(
    HWND hwnd
)
{
    RECT bounds{};

    GetClientRect(
        hwnd,
        &bounds
    );

    gState.width =
        bounds.right - bounds.left;

    gState.height =
        bounds.bottom - bounds.top;

    if (!gWebViewController)
    {
        return;
    }

    HRESULT hr =
        gWebViewController->put_Bounds(
            bounds
        );

    if (FAILED(hr))
    {
        LogHRESULT(
            "put_Bounds failed",
            hr
        );
    }
}


// ------------------------------------------------------------
// WebView2
// ------------------------------------------------------------

HRESULT InitializeWebView2()
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
                        LogHRESULT(
                            "WebView2 environment failed",
                            result
                        );

                        return result;
                    }

                    LogEvent(
                        "WebView2 environment created"
                    );

                    ComPtr<
                        ICoreWebView2Environment3
                    > environment3;

                    HRESULT hr =
                        environment->QueryInterface(
                            IID_PPV_ARGS(
                                &environment3
                            )
                        );

                    if (
                        FAILED(hr) ||
                        !environment3
                    )
                    {
                        LogHRESULT(
                            "ICoreWebView2Environment3 failed",
                            hr
                        );

                        return hr;
                    }

                    LogEvent(
                        "ICoreWebView2Environment3 acquired"
                    );

                    return environment3
                        ->CreateCoreWebView2CompositionController(
                            gWindow,

                            Callback<
                                ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler
                            >(
                                [](
                                    HRESULT result,
                                    ICoreWebView2CompositionController*
                                        compositionController
                                ) -> HRESULT
                                {
                                    if (
                                        FAILED(result) ||
                                        !compositionController
                                    )
                                    {
                                        LogHRESULT(
                                            "CompositionController creation failed",
                                            result
                                        );

                                        return result;
                                    }

                                    gCompositionController =
                                        compositionController;

                                    LogEvent(
                                        "CompositionController created"
                                    );

                                    HRESULT hr =
                                        gCompositionController.As(
                                            &gWebViewController
                                        );

                                    if (
                                        FAILED(hr) ||
                                        !gWebViewController
                                    )
                                    {
                                        LogHRESULT(
                                            "ICoreWebView2Controller acquisition failed",
                                            hr
                                        );

                                        return hr;
                                    }

                                    hr =
                                        gWebViewController
                                            ->get_CoreWebView2(
                                                &gWebView
                                            );

                                    if (
                                        FAILED(hr) ||
                                        !gWebView
                                    )
                                    {
                                        LogHRESULT(
                                            "get_CoreWebView2 failed",
                                            hr
                                        );

                                        return hr;
                                    }

                                    hr =
                                        gCompositionController
                                            ->put_RootVisualTarget(
                                                gWebViewVisual.Get()
                                            );

                                    if (FAILED(hr))
                                    {
                                        LogHRESULT(
                                            "put_RootVisualTarget failed",
                                            hr
                                        );

                                        return hr;
                                    }

                                    LogEvent(
                                        "WebView visual connected"
                                    );

                                    ComPtr<
                                        ICoreWebView2Controller2
                                    > controller2;

                                    hr =
                                        gWebViewController.As(
                                            &controller2
                                        );

                                    if (
                                        SUCCEEDED(hr) &&
                                        controller2
                                    )
                                    {
                                        COREWEBVIEW2_COLOR
                                            transparent{
                                                0,
                                                0,
                                                0,
                                                0
                                            };

                                        hr =
                                            controller2
                                                ->put_DefaultBackgroundColor(
                                                    transparent
                                                );

                                        if (FAILED(hr))
                                        {
                                            LogHRESULT(
                                                "Transparent WebView background failed",
                                                hr
                                            );
                                        }
                                        else
                                        {
                                            LogEvent(
                                                "WebView background transparent"
                                            );
                                        }
                                    }

                                    ResizeWebView(
                                        gWindow
                                    );

                                    hr =
                                        gDCompDevice->Commit();

                                    if (FAILED(hr))
                                    {
                                        LogHRESULT(
                                            "DComp WebView Commit failed",
                                            hr
                                        );

                                        return hr;
                                    }

                                    LogEvent(
                                        "Composition tree committed"
                                    );

                                    hr =
                                        gWebView->Navigate(
                                            L"file:///C:/Users/acer/Projects/"
                                            L"Hotoe-Windows/prototypes/"
                                            L"06-click-through/index.html"
                                        );

                                    if (FAILED(hr))
                                    {
                                        LogHRESULT(
                                            "Navigate failed",
                                            hr
                                        );

                                        return hr;
                                    }

                                    LogEvent(
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