#include "../runtime.h"
#include "../request.h"
#include "../response.h"

RuntimeResponse Close(
    Runtime& runtime,
    const RuntimeRequest& request
)
{
    if (
        runtime.host &&
        runtime.host->closeApplication &&
        runtime.host->closeApplication(runtime)
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
        L"Application close is unavailable"
    };
}
