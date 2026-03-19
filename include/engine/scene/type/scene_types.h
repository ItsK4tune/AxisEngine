#pragma once

#include <cstdint>

namespace scene {
    // Octree Constants
    constexpr int OCTREE_MAX_ELEMENTS = 16;
    constexpr int OCTREE_MAX_DEPTH = 6;

    // Binary Serialization Constants
    constexpr uint32_t BINARY_MAGIC = 0x41585342;
    constexpr uint32_t BINARY_VERSION = 2;
}
