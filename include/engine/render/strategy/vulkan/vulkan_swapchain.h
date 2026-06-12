#pragma once
#include <render/rhi/i_swapchain.h>
#if AXIS_HAS_VULKAN_BACKEND

namespace rhi
{
class VulkanSwapchain final : public ISwapchain
{
public:
    void Resize(uint32_t width, uint32_t height) override;
    Extent2D GetExtent() const override;
    Format GetBackBufferFormat() const override;
    ImageHandle GetCurrentBackBuffer() const override;

    void SetCurrentBackBuffer(ImageHandle image);
    void SetBackBufferFormat(Format format);

private:
    Extent2D m_Extent;
    Format m_BackBufferFormat = Format::BGRA8;
    ImageHandle m_CurrentBackBuffer;
};
}  // namespace rhi
#endif