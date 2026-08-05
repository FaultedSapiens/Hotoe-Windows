#pragma once

#include <string>

struct Runtime;

bool PushStringToBus(
    Runtime& runtime,
    const std::wstring& data
);
