#pragma once

#include <render/rhi/rhi_types.h>

namespace rhi
{
class ISwapchain
{
public:
    virtual ~ISwapchain() = default;

    virtual void Resize(uint32_t width, uint32_t height) = 0;
    virtual Extent2D GetExtent() const = 0;
    virtual Format GetBackBufferFormat() const = 0;
    virtual ImageHandle GetCurrentBackBuffer() const = 0;
};
}  // namespace rhi
