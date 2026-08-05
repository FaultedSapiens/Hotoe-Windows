#include "ping.h"

RuntimeResponse Ping(
    Runtime&,
    const RuntimeRequest& request
)
{
    return
    {
        request.id,
        true,
        L"pong",
        L""
    };
}