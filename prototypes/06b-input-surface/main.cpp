#include <windows.h>
#include <cstdio>

// ============================================================
// P6B
// ============================================================

constexpr wchar_t VISUAL_CLASS[] = L"HotoeP6BVisual";
constexpr wchar_t INPUT_CLASS[]  = L"HotoeP6BInput";

constexpr int SIR_X = 100;
constexpr int SIR_Y = 100;
constexpr int SIR_W = 400;
constexpr int SIR_H = 250;

HWND gVisualWindow = nullptr;
HWND gInputWindow  = nullptr;


// ============================================================
// Logging
// ============================================================

void Log(const char* text)
{
    std::printf("%s\n", text);
    std::fflush(stdout);
}


// ============================================================
// VISUAL WINDOW
//
// This is essentially our proven P6A.
// It must NEVER receive input.
// ============================================================

LRESULT CALLBACK VisualWindowProc(
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
// INPUT WINDOW
//
// Invisible native surface occupying ONLY the SIR.
// ============================================================

LRESULT CALLBACK InputWindowProc(
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
            static bool tracking = false;

            if (!tracking)
            {
                TRACKMOUSEEVENT tme{};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;

                TrackMouseEvent(&tme);

                tracking = true;

                Log("P6B: POINTER ENTERED SIR");
            }

            return 0;
        }


        case WM_MOUSELEAVE:
        {
            Log("P6B: POINTER LEFT SIR");

            // reset tracking state
            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_CANCEL | TME_LEAVE;
            tme.hwndTrack = hwnd;

            TrackMouseEvent(&tme);

            return 0;
        }


        case WM_LBUTTONDOWN:
        {
            Log("P6B: WM_LBUTTONDOWN");

            SetCapture(hwnd);

            return 0;
        }


        case WM_LBUTTONUP:
        {
            Log("P6B: WM_LBUTTONUP");

            if (GetCapture() == hwnd)
                ReleaseCapture();

            return 0;
        }


        case WM_RBUTTONDOWN:
        {
            Log("P6B: WM_RBUTTONDOWN");
            return 0;
        }


        case WM_RBUTTONUP:
        {
            Log("P6B: WM_RBUTTONUP");
            return 0;
        }


        case WM_MBUTTONDOWN:
        {
            Log("P6B: WM_MBUTTONDOWN");
            return 0;
        }


        case WM_MBUTTONUP:
        {
            Log("P6B: WM_MBUTTONUP");
            return 0;
        }


        case WM_MOUSEWHEEL:
        {
            Log("P6B: WM_MOUSEWHEEL");
            return 0;
        }


        case WM_MOUSEACTIVATE:
        {
            /*
                Important for P6B.

                We want pointer ownership without turning
                this invisible SIR into a normal application
                window.

                Keyboard/focus semantics come later.
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
// Register classes
// ============================================================

bool RegisterClasses(HINSTANCE instance)
{
    WNDCLASSW visual{};

    visual.lpfnWndProc   = VisualWindowProc;
    visual.hInstance     = instance;
    visual.lpszClassName = VISUAL_CLASS;
    visual.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    visual.hbrBackground = nullptr;

    if (!RegisterClassW(&visual))
    {
        Log("FAILED: Visual RegisterClassW");
        return false;
    }


    WNDCLASSW input{};

    input.lpfnWndProc   = InputWindowProc;
    input.hInstance     = instance;
    input.lpszClassName = INPUT_CLASS;
    input.hCursor       = LoadCursorW(nullptr, IDC_HAND);
    input.hbrBackground = nullptr;

    if (!RegisterClassW(&input))
    {
        Log("FAILED: Input RegisterClassW");
        return false;
    }

    return true;
}


// ============================================================
// Create visual surface
// ============================================================

bool CreateVisualSurface(HINSTANCE instance)
{
    const int width =
        GetSystemMetrics(SM_CXSCREEN);

    const int height =
        GetSystemMetrics(SM_CYSCREEN);


    gVisualWindow = CreateWindowExW(
        WS_EX_LAYERED |
        WS_EX_TRANSPARENT |
        WS_EX_NOACTIVATE |
        WS_EX_TOOLWINDOW |
        WS_EX_TOPMOST,

        VISUAL_CLASS,
        L"Hotoe P6B Visual",

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
        Log("FAILED: Create visual HWND");
        return false;
    }


    /*
        Black = transparent.

        Exactly the same mechanism proven by P6A.
    */

    if (!SetLayeredWindowAttributes(
            gVisualWindow,
            RGB(0, 0, 0),
            255,
            LWA_COLORKEY
        ))
    {
        Log("FAILED: SetLayeredWindowAttributes");
        return false;
    }


    ShowWindow(
        gVisualWindow,
        SW_SHOWNOACTIVATE
    );


    // --------------------------------------------------------
    // Draw P6B marker
    // --------------------------------------------------------

    HDC dc =
        GetDC(gVisualWindow);


    RECT screen{
        0,
        0,
        width,
        height
    };


    HBRUSH black =
        CreateSolidBrush(
            RGB(0, 0, 0)
        );


    FillRect(
        dc,
        &screen,
        black
    );


    DeleteObject(black);


    RECT sir{
        SIR_X,
        SIR_Y,
        SIR_X + SIR_W,
        SIR_Y + SIR_H
    };


    HBRUSH red =
        CreateSolidBrush(
            RGB(255, 0, 0)
        );


    FillRect(
        dc,
        &sir,
        red
    );


    DeleteObject(red);


    SetBkMode(
        dc,
        TRANSPARENT
    );


    SetTextColor(
        dc,
        RGB(255, 255, 255)
    );


    const wchar_t line1[] =
        L"HOTOE P6B";

    const wchar_t line2[] =
        L"THIS REGION OWNS INPUT";


    TextOutW(
        dc,
        130,
        190,
        line1,
        ARRAYSIZE(line1) - 1
    );


    TextOutW(
        dc,
        130,
        220,
        line2,
        ARRAYSIZE(line2) - 1
    );


    ReleaseDC(
        gVisualWindow,
        dc
    );


    Log("P6B: Visual surface created");

    return true;
}


// ============================================================
// Create input surface
// ============================================================

bool CreateInputSurface(HINSTANCE instance)
{
    /*
        Critical point:

        THIS WINDOW IS NOT FULLSCREEN.

        Its physical Win32 dimensions ARE the SIR.
    */

    gInputWindow = CreateWindowExW(
        WS_EX_NOACTIVATE |
        WS_EX_TOOLWINDOW |
        WS_EX_TOPMOST,

        INPUT_CLASS,
        L"Hotoe P6B Input",

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
        Log("FAILED: Create input HWND");
        return false;
    }


    /*
        Do not paint anything.

        The visible red rectangle belongs to the
        separate visual HWND.
    */


    ShowWindow(
        gInputWindow,
        SW_SHOWNOACTIVATE
    );


    /*
        Explicitly keep the input surface topmost without
        activating it.
    */

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


    Log("P6B: Input surface created");
    Log("P6B: Input region = (100,100) 400x250");

    return true;
}


// ============================================================
// WinMain
// ============================================================

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int
)
{
    // --------------------------------------------------------
    // Console
    // --------------------------------------------------------

    AllocConsole();

    FILE* fp = nullptr;

    freopen_s(
        &fp,
        "CONOUT$",
        "w",
        stdout
    );


    Log("========================================");
    Log("Hotoe Prototype 06B");
    Log("Visual Surface + Native Input Surface");
    Log("========================================");


    // --------------------------------------------------------
    // Classes
    // --------------------------------------------------------

    if (!RegisterClasses(instance))
        return 1;


    // --------------------------------------------------------
    // P6A visual layer
    // --------------------------------------------------------

    if (!CreateVisualSurface(instance))
        return 1;


    // --------------------------------------------------------
    // P6B input layer
    // --------------------------------------------------------

    if (!CreateInputSurface(instance))
        return 1;


    Log("");
    Log("READY");
    Log("");
    Log("Expected:");
    Log("  RED BOX    -> Hotoe receives mouse");
    Log("  EVERYWHERE ELSE -> underlying application");
    Log("");


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