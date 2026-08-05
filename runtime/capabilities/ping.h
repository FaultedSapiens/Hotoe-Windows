#pragma once

#include "../runtime.h"
#include "../request.h"
#include "../response.h"

RuntimeResponse Ping(
    Runtime&,
    const RuntimeRequest&
);