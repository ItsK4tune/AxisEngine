#include <render/strategy/vulkan/vulkan_swapchain.h>
#if AXIS_HAS_VULKAN_BACKEND
namespace rhi
{
void VulkanSwapchain::Resize(uint32_t width, uint32_t height)
{
    m_Extent = {width, height};
}

Extent2D VulkanSwapchain::GetExtent() const
{
    return m_Extent;
}

Format VulkanSwapchain::GetBackBufferFormat() const
{
    return m_BackBufferFormat;
}

ImageHandle VulkanSwapchain::GetCurrentBackBuffer() const
{
    return m_CurrentBackBuffer;
}

void VulkanSwapchain::SetCurrentBackBuffer(ImageHandle image)
{
    m_CurrentBackBuffer = image;
}

void VulkanSwapchain::SetBackBufferFormat(Format format)
{
    m_BackBufferFormat = format;
}

}  // namespace rhi
#endif