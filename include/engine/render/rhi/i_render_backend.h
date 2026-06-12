#pragma once

#include <render/rhi/i_render_device.h>
#include <render/rhi/i_swapchain.h>
#include <render/rhi/rhi_objects.h>
#include <render/rhi/rhi_types.h>

namespace rhi
{
class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    virtual bool Initialize(const RenderBackendCreateInfo& createInfo) = 0;
    virtual void Shutdown() = 0;

    virtual bool BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void OnResize(uint32_t width, uint32_t height) = 0;

    virtual IRenderDevice& GetDevice() = 0;
    virtual ISwapchain& GetSwapchain() = 0;

    virtual BackendType GetBackendType() const = 0;
    virtual const char* GetName() const = 0;

    virtual void ImGuiInit(void* /*window*/) {}
    virtual void ImGuiShutdown() {}
    virtual void ImGuiNewFrame() {}
    virtual void ImGuiRender() {}
};
}  // namespace rhi
