#include <render/strategy/vulkan/vulkan_render_backend.h>
#if AXIS_HAS_VULKAN_BACKEND
#include <render/strategy/vulkan/vulkan_utils.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <set>
#include <array>
#include <limits>

#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#endif

namespace rhi
{
VulkanRenderBackend::VulkanRenderBackend()
{
}

VulkanRenderBackend::~VulkanRenderBackend()
{
    Shutdown();
}

bool VulkanRenderBackend::Initialize(const RenderBackendCreateInfo& createInfo)
{
    if (m_Initialized)
        return true;

    m_Vsync = createInfo.vsync;

    if (!CreateInstance(createInfo))
        return false;
    if (createInfo.nativeWindow && !CreateSurface(createInfo.nativeWindow))
        return false;
    if (!PickPhysicalDevice())
        return false;
    if (!CreateDevice())
        return false;
    if (!m_RenderDevice.Initialize(m_Instance, m_PhysicalDevice, m_Device, m_GraphicsQueue, m_GraphicsQueueFamily))
        return false;
    if (m_Surface && !CreateSwapchain(createInfo.width, createInfo.height))
        return false;

    m_Initialized = true;
    return true;
}

void VulkanRenderBackend::Shutdown()
{
    if (!m_Instance)
        return;

    if (m_Device)
        vkDeviceWaitIdle(m_Device);
    DestroySwapchain();
    m_RenderDevice.Shutdown();
    if (m_ImageAvailableFence)
    {
        vkDestroyFence(m_Device, m_ImageAvailableFence, nullptr);
        m_ImageAvailableFence = VK_NULL_HANDLE;
    }
    if (m_Device)
    {
        vkDestroyDevice(m_Device, nullptr);
        m_Device = VK_NULL_HANDLE;
    }
    if (m_Surface)
    {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        m_Surface = VK_NULL_HANDLE;
    }
    vkDestroyInstance(m_Instance, nullptr);
    m_Instance = VK_NULL_HANDLE;
    m_Initialized = false;
}

bool VulkanRenderBackend::BeginFrame()
{
    if (!m_Initialized)
        return false;
    if (m_SwapchainHandle)
    {
        if (m_ImageAvailableFence)
            vkResetFences(m_Device, 1, &m_ImageAvailableFence);

        VkResult result =
            vkAcquireNextImageKHR(m_Device, m_SwapchainHandle, std::numeric_limits<uint64_t>::max(), VK_NULL_HANDLE,
                                  m_ImageAvailableFence, &m_CurrentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            return false;
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            return false;

        if (m_ImageAvailableFence)
            vkWaitForFences(m_Device, 1, &m_ImageAvailableFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

        if (m_CurrentImageIndex < m_SwapchainImageHandles.size())
            m_Swapchain.SetCurrentBackBuffer(m_SwapchainImageHandles[m_CurrentImageIndex]);
    }
    return true;
}

void VulkanRenderBackend::EndFrame()
{
    if (!m_SwapchainHandle)
        return;
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_SwapchainHandle;
    presentInfo.pImageIndices = &m_CurrentImageIndex;
    VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR)
        Check(result, "vkQueuePresentKHR");
    vkQueueWaitIdle(m_PresentQueue);
}

void VulkanRenderBackend::OnResize(uint32_t width, uint32_t height)
{
    if (!m_Device)
        return;
    vkDeviceWaitIdle(m_Device);
    DestroySwapchain();
    if (m_Surface)
        CreateSwapchain(width, height);
    m_Swapchain.Resize(width, height);
}

IRenderDevice& VulkanRenderBackend::GetDevice()
{
    return m_RenderDevice;
}

ISwapchain& VulkanRenderBackend::GetSwapchain()
{
    return m_Swapchain;
}

BackendType VulkanRenderBackend::GetBackendType() const
{
    return BackendType::Vulkan;
}

const char* VulkanRenderBackend::GetName() const
{
    return "Vulkan";
}

bool VulkanRenderBackend::CreateInstance(const RenderBackendCreateInfo& createInfo)
{
    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = createInfo.applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "AxisEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    VkInstanceCreateInfo createInfoVk{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfoVk.pApplicationInfo = &appInfo;
    createInfoVk.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfoVk.ppEnabledExtensionNames = extensions.data();

    std::array<const char*, 1> validationLayers = {{"VK_LAYER_KHRONOS_validation"}};
    if (createInfo.enableValidation)
    {
        createInfoVk.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfoVk.ppEnabledLayerNames = validationLayers.data();
    }

    return Check(vkCreateInstance(&createInfoVk, nullptr, &m_Instance), "vkCreateInstance");
}

bool VulkanRenderBackend::CreateSurface(void* nativeWindow)
{
    return Check(glfwCreateWindowSurface(m_Instance, static_cast<GLFWwindow*>(nativeWindow), nullptr, &m_Surface),
                 "glfwCreateWindowSurface");
}

bool VulkanRenderBackend::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        return false;

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

    for (VkPhysicalDevice device : devices)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
                continue;
            VkBool32 presentSupport = m_Surface ? VK_FALSE : VK_TRUE;
            if (m_Surface)
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
            if (presentSupport)
            {
                m_PhysicalDevice = device;
                m_GraphicsQueueFamily = i;
                m_PresentQueueFamily = i;
                return true;
            }
        }
    }
    return false;
}

bool VulkanRenderBackend::CreateDevice()
{
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = m_GraphicsQueueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    std::vector<const char*> extensions;
    if (m_Surface)
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures = &features;

    if (!Check(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "vkCreateDevice"))
        return false;
    vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
    m_PresentQueue = m_GraphicsQueue;

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    return Check(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_ImageAvailableFence), "vkCreateFence(ImageAcquire)");
}

bool VulkanRenderBackend::CreateSwapchain(uint32_t width, uint32_t height)
{
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data());

    VkSurfaceFormatKHR chosen = formats.empty()
                                    ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                                    : formats.front();
    for (const auto& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            chosen = format;
            break;
        }
    }
    m_SwapchainFormat = chosen.format;

    VkExtent2D extent{};
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        extent = capabilities.currentExtent;
    }
    else
    {
        extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, capabilities.maxImageCount);

    m_MinImageCount = capabilities.minImageCount;

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!m_Vsync)
    {
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, presentModes.data());

        bool foundMailbox = false;
        bool foundImmediate = false;
        for (auto mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                foundMailbox = true;
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                foundImmediate = true;
        }
        if (foundMailbox)
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (foundImmediate)
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    VkSwapchainCreateInfoKHR swapchainInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = m_Surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = chosen.format;
    swapchainInfo.imageColorSpace = chosen.colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;

    if (!Check(vkCreateSwapchainKHR(m_Device, &swapchainInfo, nullptr, &m_SwapchainHandle), "vkCreateSwapchainKHR"))
        return false;

    vkGetSwapchainImagesKHR(m_Device, m_SwapchainHandle, &imageCount, nullptr);
    m_SwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_SwapchainHandle, &imageCount, m_SwapchainImages.data());

    m_SwapchainImageHandles.clear();
    for (auto image : m_SwapchainImages)
    {
        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = chosen.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView view = VK_NULL_HANDLE;
        if (!Check(vkCreateImageView(m_Device, &viewInfo, nullptr, &view), "vkCreateImageView(Swapchain)"))
            return false;

        uint32_t id = m_RenderDevice.AllocateHandle();
        ImageDesc desc;
        desc.width = extent.width;
        desc.height = extent.height;
        desc.format = FromVkFormat(chosen.format);
        desc.usage = ImageUsage::Present | ImageUsage::ColorAttachment;
        m_RenderDevice.m_Images[id] =
            VulkanRenderDevice::ImageResource{image, view, VK_NULL_HANDLE, desc, VK_IMAGE_LAYOUT_UNDEFINED, false};
        m_SwapchainImageHandles.push_back(ImageHandle{id});
    }

    m_Swapchain.Resize(extent.width, extent.height);
    m_Swapchain.SetBackBufferFormat(FromVkFormat(chosen.format));
    return true;
}

void VulkanRenderBackend::DestroySwapchain()
{
    for (auto handle : m_SwapchainImageHandles)
    {
        auto it = m_RenderDevice.m_Images.find(handle.id);
        if (it != m_RenderDevice.m_Images.end())
        {
            if (it->second.view)
                vkDestroyImageView(m_Device, it->second.view, nullptr);
            m_RenderDevice.m_Images.erase(it);
        }
    }
    m_SwapchainImageHandles.clear();
    m_SwapchainImages.clear();
    if (m_SwapchainHandle)
    {
        vkDestroySwapchainKHR(m_Device, m_SwapchainHandle, nullptr);
        m_SwapchainHandle = VK_NULL_HANDLE;
    }
}

void VulkanRenderBackend::ImGuiInit(void* window)
{
#ifdef ENABLE_EDITOR
    (void)window;
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_2;
    init_info.Instance = m_Instance;
    init_info.PhysicalDevice = m_PhysicalDevice;
    init_info.Device = m_Device;
    init_info.QueueFamily = m_GraphicsQueueFamily;
    init_info.Queue = m_GraphicsQueue;
    init_info.DescriptorPool = VK_NULL_HANDLE;
    init_info.DescriptorPoolSize = 256;
    init_info.MinImageCount = m_MinImageCount;
    init_info.ImageCount = static_cast<uint32_t>(m_SwapchainImages.size());
    init_info.UseDynamicRendering = false;

    RenderTargetLayoutDesc layoutDesc;
    layoutDesc.colorFormats.push_back(FromVkFormat(m_SwapchainFormat));
    VkRenderPass renderPass = m_RenderDevice.GetOrCreateRenderPass(layoutDesc);
    init_info.PipelineInfoMain.RenderPass = renderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);
#endif
}

void VulkanRenderBackend::ImGuiShutdown()
{
#ifdef ENABLE_EDITOR
    ImGui_ImplVulkan_Shutdown();
#endif
}

void VulkanRenderBackend::ImGuiNewFrame()
{
#ifdef ENABLE_EDITOR
    ImGui_ImplVulkan_NewFrame();
#endif
}

void VulkanRenderBackend::ImGuiRender()
{
#ifdef ENABLE_EDITOR
    auto* drawData = ImGui::GetDrawData();
    if (!drawData)
        return;

    ImageHandle backBuffer = m_Swapchain.GetCurrentBackBuffer();
    if (!backBuffer)
        return;

    Extent2D extent = m_Swapchain.GetExtent();
    if (extent.width == 0 || extent.height == 0)
        return;

    auto& commandList = m_RenderDevice.BeginCommandList(CommandQueueType::Graphics);

    RenderAttachmentDesc colorAttachment;
    colorAttachment.image = backBuffer;
    colorAttachment.format = m_Swapchain.GetBackBufferFormat();
    colorAttachment.loadOp = LoadOp::Load;
    colorAttachment.storeOp = StoreOp::Store;

    RenderPassBeginInfo beginInfo;
    beginInfo.colorAttachments.push_back(colorAttachment);
    beginInfo.renderArea = {0, 0, extent.width, extent.height};

    commandList.BeginRendering(beginInfo);

    auto* vkCmdList = dynamic_cast<VulkanCommandList*>(&commandList);
    if (vkCmdList)
    {
        VkCommandBuffer cmdBuffer = vkCmdList->GetNative();
        ImGui_ImplVulkan_RenderDrawData(drawData, cmdBuffer);
    }

    commandList.EndRendering();
    m_RenderDevice.Submit(commandList);
#endif
}
}  // namespace rhi
#endif
