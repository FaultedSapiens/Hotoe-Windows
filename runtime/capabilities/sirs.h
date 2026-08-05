#pragma once

#include "../runtime.h"
#include "../request.h"
#include "../response.h"

RuntimeResponse RecalculateInputRegions(
    Runtime&,
    const RuntimeRequest&
);