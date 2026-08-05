#pragma once

#include "../runtime.h"
#include "../request.h"
#include "../response.h"

RuntimeResponse Version(
    Runtime&,
    const RuntimeRequest&
);