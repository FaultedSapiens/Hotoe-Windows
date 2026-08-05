#pragma once

struct Runtime;
struct RuntimeIpcMessage;

struct RuntimeIpcBackend
{
    bool (*pushString)(Runtime&, const RuntimeIpcMessage&) = nullptr;
};
