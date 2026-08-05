#include "response.h"

std::wstring SerializeResponse(
    const RuntimeResponse& response
)
{
    if (response.ok)
    {
        return
            L"{\"id\":"
            + std::to_wstring(response.id)
            + L",\"ok\":true,\"result\":\""
            + response.result
            + L"\"}";
    }

    return
        L"{\"id\":"
        + std::to_wstring(response.id)
        + L",\"ok\":false,\"error\":\""
        + response.error
        + L"\"}";
}