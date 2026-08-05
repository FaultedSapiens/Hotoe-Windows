#include "capabilities.h"

const Capability gCapabilities[] =
{
    { L"fx.ping",                      Ping                     },
    { L"fx.version",                   Version                  },
    { L"fx.close",                     Close                    },

    { L"fx.closeApplication",          Close                    },
    { L"fx.recalculateInputRegions",   RecalculateInputRegions  }
};

const size_t gCapabilityCount =
    sizeof(gCapabilities) /
    sizeof(gCapabilities[0]);
