#pragma once

struct Manifest;
struct Runtime;

struct RuntimeHost
{
    bool (*closeApplication)(Runtime&) = nullptr;
    bool (*recalculateInputRegions)(Runtime&) = nullptr;
    bool (*dispatchFocusEvent)(Runtime&, bool) = nullptr;
};

struct Runtime
{
    Manifest* manifest = nullptr;
    RuntimeHost* host = nullptr;
};
