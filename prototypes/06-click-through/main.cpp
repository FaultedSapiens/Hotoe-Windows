#include <windows.h>
#include <objbase.h>

#include "state.h"
#include "composition.h"
#include "window.h"

#include <cstdio>
#include <iostream>
#include <iomanip>

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static void ShowFatalError(
    const wchar_t* message
)
{
    MessageBoxW(
        nullptr,
        message,
        L"Hotoe Prototype 06",
        MB_OK | MB_ICONERROR
    );
}

static void LogHRESULT(
    const char* operation,
    HRESULT hr
)
{
    std::cout
        << operation
        << " failed. HRESULT=0x"
        << std::hex
        << std::uppercase
        << static_cast<unsigned long>(hr)
        << std::nouppercase
        << std::dec
        << '\n';
}

// ------------------------------------------------------------
// Entry point
// ------------------------------------------------------------

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int
)
{
    // --------------------------------------------------------
    // COM
    //
    // WebView2 is COM-based. The UI thread must initialize
    // COM before creating the WebView2 environment.
    // --------------------------------------------------------

    HRESULT comHr = CoInitializeEx(
        nullptr,
        COINIT_APARTMENTTHREADED
    );

    if (FAILED(comHr))
    {
        ShowFatalError(
            L"Failed to initialize COM."
        );

        return 1;
    }

    // --------------------------------------------------------
    // Debug console
    // --------------------------------------------------------

    if (AllocConsole())
    {
        FILE* fp = nullptr;

        freopen_s(
            &fp,
            "CONOUT$",
            "w",
            stdout
        );

        freopen_s(
            &fp,
            "CONOUT$",
            "w",
            stderr
        );
    }

    LogEvent(
        "=== Hotoe Prototype 06 / Split Surface ==="
    );

    LogEvent(
        "COM initialized"
    );

    // --------------------------------------------------------
    // Register visual HWND class
    // --------------------------------------------------------

    if (!RegisterHotoeWindowClass(
            hInstance
        ))
    {
        LogEvent(
            "RegisterHotoeWindowClass FAILED"
        );

        ShowFatalError(
            L"Failed to register the Hotoe visual window class."
        );

        CoUninitialize();

        return 1;
    }

    // --------------------------------------------------------
    // Create visual HWND
    //
    // IMPORTANT:
    //
    // This HWND is ONLY our visual surface.
    //
    // It must NOT receive SetWindowRgn().
    // It must NOT own the future SIR input region.
    //
    // Input gets its own HWND in input.cpp.
    // --------------------------------------------------------

    if (!CreateHotoeWindow(
            hInstance
        ))
    {
        LogEvent(
            "CreateHotoeWindow FAILED"
        );

        ShowFatalError(
            L"Failed to create the Hotoe visual window."
        );

        CoUninitialize();

        return 1;
    }

    LogEvent(
        "Visual surface created"
    );

    // --------------------------------------------------------
    // DirectComposition
    // --------------------------------------------------------

    if (!InitializeDirectComposition(
            gWindow
        ))
    {
        LogEvent(
            "InitializeDirectComposition FAILED"
        );

        ShowFatalError(
            L"Failed to initialize DirectComposition."
        );

        DestroyWindow(
            gWindow
        );

        gWindow = nullptr;

        CoUninitialize();

        return 1;
    }

    LogEvent(
        "Visual composition ready"
    );

    // --------------------------------------------------------
    // WebView2
    //
    // InitializeWebView2() starts asynchronous WebView2
    // creation. Completion continues through its callbacks.
    // --------------------------------------------------------

    HRESULT webViewHr =
        InitializeWebView2();

    if (FAILED(webViewHr))
    {
        LogHRESULT(
            "InitializeWebView2",
            webViewHr
        );

        ShowFatalError(
            L"Failed to begin WebView2 initialization."
        );

        DestroyWindow(
            gWindow
        );

        gWindow = nullptr;

        CoUninitialize();

        return 1;
    }

    LogEvent(
        "WebView2 initialization requested"
    );

    // --------------------------------------------------------
    // Message loop
    //
    // WebView2 creation is asynchronous, therefore this loop
    // must remain alive while its callbacks execute.
    // --------------------------------------------------------

    MSG message{};

    BOOL result = 0;

    while (
        (result = GetMessageW(
            &message,
            nullptr,
            0,
            0
        )) != 0
    )
    {
        if (result == -1)
        {
            LogEvent(
                "GetMessageW FAILED"
            );

            break;
        }

        TranslateMessage(
            &message
        );

        DispatchMessageW(
            &message
        );
    }

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    LogEvent(
        "Hotoe P6 shutting down"
    );

    /*
        Release WebView/DirectComposition objects while COM
        is still initialized.

        These globals are declared in composition.cpp.
    */

    gWindow = nullptr;

CoUninitialize();

return 0;
}