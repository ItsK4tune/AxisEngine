#pragma once

#include <cstdint>

struct OcclusionComponent {
    uint32_t lastQueryId = 0;
    bool isVisible = true;
    bool queryPending = false;
};
