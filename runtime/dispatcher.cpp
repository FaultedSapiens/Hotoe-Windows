#include "dispatcher.h"

#include "capabilities/capabilities.h"

const Capability* FindCapability(
    const std::wstring& name
)
{
    for(size_t i=0;i<gCapabilityCount;i++)
    {
        if(name==gCapabilities[i].name)
            return &gCapabilities[i];
    }

    return nullptr;
}

RuntimeResponse InvokeCapability(
    Runtime& runtime,
    const RuntimeRequest& request
)
{
    const Capability* capability =
        FindCapability(
            request.capability
        );

    if(!capability)
    {
        return
        {
            request.id,
            false,
            L"",
            L"Unknown capability"
        };
    }

    return capability->function(
        runtime,
        request
    );
}