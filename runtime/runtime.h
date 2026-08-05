#pragma once

struct Manifest;
struct Runtime;
struct RuntimeIpcBackend;

struct RuntimeHost
{
    bool (*closeApplication)(Runtime&) = nullptr;
    bool (*recalculateInputRegions)(Runtime&) = nullptr;
    bool (*dispatchFocusEvent)(Runtime&, bool) = nullptr;
    bool (*dispatchBusMessage)(Runtime&, const wchar_t*) = nullptr;

    RuntimeIpcBackend* ipcBackend = nullptr;
};

struct Runtime
{
    Manifest* manifest = nullptr;
    RuntimeHost* host = nullptr;
};
