#pragma once

struct Runtime;
struct RuntimeRequest;
struct RuntimeResponse;

using CapabilityFunction =
RuntimeResponse(*)(
    Runtime&,
    const RuntimeRequest&
);

struct Capability
{
    const wchar_t* name;

    CapabilityFunction function;
};