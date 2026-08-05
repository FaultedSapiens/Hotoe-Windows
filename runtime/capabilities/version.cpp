#include "../runtime.h"
#include "../request.h"
#include "../response.h"

RuntimeResponse Version(
    Runtime&,
    const RuntimeRequest& request
)
{
    return
    {
        request.id,
        true,
        L"0.1",
        L""
    };
}