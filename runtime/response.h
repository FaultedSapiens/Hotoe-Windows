#pragma once

#include <string>

struct RuntimeResponse
{
    int id = 0;

    bool ok = false;

    std::wstring result;

    std::wstring error;
};

std::wstring SerializeResponse(
    const RuntimeResponse& response
);