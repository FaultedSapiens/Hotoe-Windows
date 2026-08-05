#include "window.h"

#include "state.h"
#include "composition.h"

#include <string>

constexpr wchar_t WINDOW_CLASS[] =
    L"HotoeWindow";

constexpr wchar_t WINDOW_TITLE[] =
    L"Hotoe Prototype 06 - Visual Surface";

HWND gWindow = nullptr;


bool RegisterHotoeWindowClass(
    HINSTANCE hInstance
)
{
    WNDCLASSW wc{};

    wc.lpfnWndProc =
        WindowProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        WINDOW_CLASS;

    wc.hCursor =
        LoadCursorW(
            nullptr,
            IDC_ARROW
        );

    // DirectComposition supplies the pixels.
    wc.hbrBackground =
        nullptr;

    if (!RegisterClassW(&wc))
    {
        LogEvent(
            "Visual RegisterClassW FAILED"
        );

        return false;
    }

    LogEvent(
        "Visual window class registered"
    );

    return true;
}


HWND CreateHotoeWindow(
    HINSTANCE hInstance
)
{
    const int screenWidth =
        GetSystemMetrics(
            SM_CXSCREEN
        );

    const int screenHeight =
        GetSystemMetrics(
            SM_CYSCREEN
        );

    gWindow =
        CreateWindowExW(
            // No DWM redirection bitmap.
            //
            // TOOLWINDOW keeps this prototype out of
            // Alt+Tab/taskbar-style normal app behavior.
            WS_EX_NOREDIRECTIONBITMAP |
            WS_EX_TOOLWINDOW |
            WS_EX_TRANSPARENT |
            WS_EX_NOACTIVATE,

            WINDOW_CLASS,
            WINDOW_TITLE,

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

    if (!gWindow)
    {
        LogEvent(
            "Visual CreateWindowExW FAILED"
        );

        return nullptr;
    }

    /*
        Critical:

        This HWND is the VISUAL surface.

        There is deliberately NO SetWindowRgn() here.

        Keep the visual surface above ordinary windows even
        while another application owns foreground/focus.
    */
    if (!SetWindowPos(
            gWindow,
            HWND_TOPMOST,

            0,
            0,
            screenWidth,
            screenHeight,

            SWP_NOACTIVATE |
            SWP_SHOWWINDOW
        ))
    {
        LogEvent(
            "Visual SetWindowPos TOPMOST FAILED"
        );

        DestroyWindow(
            gWindow
        );

        gWindow = nullptr;

        return nullptr;
    }

    LogEvent(
        "Visual HWND created -> TOPMOST / NO REGION"
    );

    return gWindow;
}


LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (message)
    {
        // --------------------------------------------
        // The visual HWND must not become our
        // interactive surface.
        // --------------------------------------------

        case WM_NCHITTEST:
        {
            return HTTRANSPARENT;
        }

        case WM_MOUSEACTIVATE:
        {
            return MA_NOACTIVATE;
        }

        // --------------------------------------------
        // State
        // --------------------------------------------

        case WM_SIZE:
        {
            ResizeWebView(
                hwnd
            );

            gState.minimized =
                wParam == SIZE_MINIMIZED;

            gState.maximized =
                wParam == SIZE_MAXIMIZED;

            LogEvent(
                "Visual WM_SIZE"
            );

            return 0;
        }

        case WM_ACTIVATE:
        {
            const WORD state =
                LOWORD(wParam);

            gState.active =
                state != WA_INACTIVE;

            gState.foreground =
                GetForegroundWindow() == hwnd;

            if (gState.active)
            {
                gState.activates++;
            }

            if (state == WA_INACTIVE)
            {
                LogEvent(
                    "Visual WM_ACTIVATE (INACTIVE)"
                );
            }
            else
            {
                LogEvent(
                    "Visual WM_ACTIVATE (ACTIVE)"
                );
            }

            return 0;
        }

        case WM_ACTIVATEAPP:
        {
            LogEvent(
                std::string(
                    "Visual WM_ACTIVATEAPP: "
                ) +
                (
                    wParam
                        ? "TRUE"
                        : "FALSE"
                )
            );

            return 0;
        }

        // --------------------------------------------
        // DirectComposition HWND:
        // don't erase a GDI background.
        // --------------------------------------------

        case WM_ERASEBKGND:
        {
            return 1;
        }

        case WM_DESTROY:
        {
            LogEvent(
                "Visual HWND destroyed"
            );

            PostQuitMessage(
                0
            );

            return 0;
        }
    }

    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}