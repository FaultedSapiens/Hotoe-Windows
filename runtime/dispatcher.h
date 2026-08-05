#pragma once

#include <cstddef>
#include <string>

#include "runtime.h"
#include "capability.h"
#include "request.h"
#include "response.h"

const Capability* FindCapability(
    const std::wstring& name
);

RuntimeResponse InvokeCapability(
    Runtime& runtime,
    const RuntimeRequest& request
);