#pragma once
#include <render/rhi/i_render_backend.h>
#include <render/strategy/directx/directx12_common.h>
#include <render/strategy/directx/directx12_render_device.h>
#include <render/strategy/directx/directx12_swapchain.h>
#if AXIS_HAS_DIRECTX_BACKEND
#include <vector>

namespace rhi
{
class DirectX12RenderBackend final : public IRenderBackend
{
public:
    DirectX12RenderBackend();
    ~DirectX12RenderBackend() override;

    bool Initialize(const RenderBackendCreateInfo& createInfo) override;
    void Shutdown() override;

    bool BeginFrame() override;
    void EndFrame() override;
    void OnResize(uint32_t width, uint32_t height) override;

    IRenderDevice& GetDevice() override;
    ISwapchain& GetSwapchain() override;

    BackendType GetBackendType() const override;
    const char* GetName() const override;

    void ImGuiInit(void* window) override;
    void ImGuiShutdown() override;
    void ImGuiNewFrame() override;
    void ImGuiRender() override;

private:
    bool CreateFactory(bool enableValidation);
    bool PickAdapter();
    bool CreateDeviceAndQueue();
    bool CreateSwapchain(void* nativeWindow, uint32_t width, uint32_t height);
    void DestroySwapchain();

    Microsoft::WRL::ComPtr<IDXGIFactory6> m_Factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_Adapter;
    Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_GraphicsQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapchainHandle;
    std::vector<ImageHandle> m_SwapchainImageHandles;
    DXGI_FORMAT m_SwapchainFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    uint32_t m_CurrentBackBuffer = 0;
    void* m_NativeWindow = nullptr;

    DirectX12RenderDevice m_RenderDevice;
    DirectX12Swapchain m_Swapchain;
    bool m_Initialized = false;
    bool m_ImGuiInitialized = false;
    bool m_Vsync = true;
};
}  // namespace rhi
#endif
