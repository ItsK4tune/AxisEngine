#include <render/strategy/directx/directx12_swapchain.h>
#if AXIS_HAS_DIRECTX_BACKEND
namespace rhi
{
void DirectX12Swapchain::Resize(uint32_t width, uint32_t height)
{
    m_Extent = {width, height};
}

Extent2D DirectX12Swapchain::GetExtent() const
{
    return m_Extent;
}

Format DirectX12Swapchain::GetBackBufferFormat() const
{
    return m_BackBufferFormat;
}

ImageHandle DirectX12Swapchain::GetCurrentBackBuffer() const
{
    return m_CurrentBackBuffer;
}

void DirectX12Swapchain::SetCurrentBackBuffer(ImageHandle image)
{
    m_CurrentBackBuffer = image;
}

void DirectX12Swapchain::SetBackBufferFormat(Format format)
{
    m_BackBufferFormat = format;
}

}  // namespace rhi
#endif