#include "sirs.h"

RuntimeResponse RecalculateInputRegions(
    Runtime& runtime,
    const RuntimeRequest& request
)
{
    if (
        runtime.host &&
        runtime.host->recalculateInputRegions &&
        runtime.host->recalculateInputRegions(runtime)
    )
    {
        return
        {
            request.id,
            true,
            L"requested",
            L""
        };
    }

    return
    {
        request.id,
        false,
        L"",
        L"Input-region recalculation is unavailable"
    };
}
