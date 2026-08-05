#include "../runtime.h"
#include "../request.h"
#include "../response.h"

RuntimeResponse Close(
    Runtime&,
    const RuntimeRequest& request
)
{
    return
    {
        request.id,
        false,
        L"",
        L"Not implemented"
    };
}