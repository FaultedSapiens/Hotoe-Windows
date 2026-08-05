#include "capabilities.h"

#include "ping.h"
#include "version.h"
#include "close.h"

const Capability gCapabilities[] =
{
    { L"fx.ping",    Ping    },
    { L"fx.version", Version },
    { L"fx.close",   Close   }
};

const size_t gCapabilityCount =
    sizeof(gCapabilities) /
    sizeof(gCapabilities[0]);