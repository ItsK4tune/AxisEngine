#pragma once

struct NetworkComponent
{
    uint32_t networkId = 0;
    uint32_t ownerId = 0;
    bool isLocal = false;
    bool replicateTransform = true;
    // Zero inherits the global policy; a positive value enables distance-based interest management.
    float interestRadius = 0.0f;
};
