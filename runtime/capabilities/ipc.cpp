#include "ipc.h"

#include "../ipc/bus.h"

RuntimeResponse PushString(
    Runtime& runtime,
    const RuntimeRequest& request
)
{
    std::wstring data;

    if (
        !request.GetString(
            L"data",
            data
        )
    )
    {
        return
        {
            request.id,
            false,
            L"",
            L"Missing IPC string data"
        };
    }

    if (
        PushStringToBus(
            runtime,
            data
        )
    )
    {
        return
        {
            request.id,
            true,
            L"",
            L""
        };
    }

    return
    {
        request.id,
        false,
        L"",
        L"IPC backend is unavailable"
    };
}
