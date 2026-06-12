#pragma once
#include <render/rhi/i_render_backend.h>
#include <render/strategy/vulkan/vulkan_render_device.h>
#include <render/strategy/vulkan/vulkan_swapchain.h>
#if AXIS_HAS_VULKAN_BACKEND
#include <vulkan/vulkan.h>
#include <vector>

namespace rhi
{
class VulkanRenderBackend final : public IRenderBackend
{
public:
    VulkanRenderBackend();
    ~VulkanRenderBackend() override;

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
    bool CreateInstance(const RenderBackendCreateInfo& createInfo);
    bool CreateSurface(void* nativeWindow);
    bool PickPhysicalDevice();
    bool CreateDevice();
    bool CreateSwapchain(uint32_t width, uint32_t height);
    void DestroySwapchain();

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamily = 0;
    uint32_t m_PresentQueueFamily = 0;
    VkSwapchainKHR m_SwapchainHandle = VK_NULL_HANDLE;
    VkFormat m_SwapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkFence m_ImageAvailableFence = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<ImageHandle> m_SwapchainImageHandles;
    uint32_t m_CurrentImageIndex = 0;
    uint32_t m_MinImageCount = 2;

    VulkanRenderDevice m_RenderDevice;
    VulkanSwapchain m_Swapchain;
    bool m_Initialized = false;
    bool m_Vsync = true;
};
}  // namespace rhi
#endif
