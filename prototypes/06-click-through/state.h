#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

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

struct InspectorEvent
{
    SYSTEMTIME time{};
    std::string text;
};

extern EngineState gState;
extern std::deque<InspectorEvent> gEventLog;

constexpr size_t MAX_LOG_ENTRIES = 100;

void LogEvent(const std::string& text);
void UpdateModifierKeys();