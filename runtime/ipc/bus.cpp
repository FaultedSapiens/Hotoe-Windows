#include "bus.h"

#include "../runtime.h"
#include "backend.h"
#include "message.h"

bool PushStringToBus(
    Runtime& runtime,
    const std::wstring& data
)
{
    if (
        !runtime.host ||
        !runtime.host->ipcBackend ||
        !runtime.host->ipcBackend->pushString
    )
    {
        return false;
    }

    RuntimeIpcMessage message{};
    message.data = data;

    return runtime.host->ipcBackend->pushString(
        runtime,
        message
    );
}
