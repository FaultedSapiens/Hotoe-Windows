#pragma once

#include <string>

struct RuntimeRequest
{
    int id = 0;

    std::wstring capability;

    std::wstring json;
};