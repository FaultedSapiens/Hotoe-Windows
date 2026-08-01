#include "state.h"

#include <iostream>

EngineState gState;
std::deque<InspectorEvent> gEventLog;

void LogEvent(const std::string& text)
{
    std::cout << text << '\n';

    InspectorEvent event{};

    GetLocalTime(&event.time);

    event.text = text;

    gEventLog.push_front(event);

    if (gEventLog.size() > MAX_LOG_ENTRIES)
    {
        gEventLog.pop_back();
    }
}

void UpdateModifierKeys()
{
    gState.ctrl =
        (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    gState.shift =
        (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    gState.alt =
        (GetKeyState(VK_MENU) & 0x8000) != 0;
}