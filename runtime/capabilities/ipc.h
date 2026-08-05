#pragma once

#include "../runtime.h"
#include "../request.h"
#include "../response.h"

RuntimeResponse PushString(
    Runtime&,
    const RuntimeRequest&
);
