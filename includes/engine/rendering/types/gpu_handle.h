#pragma once

#include <cstdint>

namespace Graphics {

    struct GpuHandle {
        uint32_t id = 0;
        bool IsValid() const { return id != 0; }
        void Reset() { id = 0; }
        bool operator==(const GpuHandle& other) const { return id == other.id; }
        bool operator!=(const GpuHandle& other) const { return id != other.id; }
    };
}
