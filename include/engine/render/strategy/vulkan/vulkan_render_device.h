#pragma once
#include <render/rhi/i_render_device.h>
#include <render/strategy/vulkan/vulkan_command_list.h>
#if AXIS_HAS_VULKAN_BACKEND
#include <vulkan/vulkan.h>
#include <map>
#include <unordered_map>

namespace rhi
{
class VulkanRenderDevice final : public IRenderDevice
{
public:
    VulkanRenderDevice();
    ~VulkanRenderDevice() override;

    bool Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue,
                    uint32_t graphicsQueueFamily);
    void Shutdown();

    BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) override;
    void UpdateBuffer(BufferHandle handle, uint64_t offset, uint64_t size, const void* data) override;
    void DestroyBuffer(BufferHandle handle) override;

    ImageHandle CreateImage(const ImageDesc& desc, const void* initialData = nullptr) override;
    void DestroyImage(ImageHandle handle) override;

    SamplerHandle CreateSampler(const SamplerDesc& desc) override;
    void DestroySampler(SamplerHandle handle) override;

    ShaderModuleHandle CreateShaderModule(const ShaderModuleDesc& desc) override;
    void DestroyShaderModule(ShaderModuleHandle handle) override;

    DescriptorSetLayoutHandle CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) override;
    void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle handle) override;

    DescriptorSetHandle CreateDescriptorSet(DescriptorSetLayoutHandle layout) override;
    void UpdateDescriptorSet(DescriptorSetHandle set, const DescriptorUpdate* updates, uint32_t count) override;
    void DestroyDescriptorSet(DescriptorSetHandle handle) override;

    PipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;
    PipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) override;
    void DestroyPipeline(PipelineHandle handle) override;

    ICommandList& BeginCommandList(CommandQueueType queue = CommandQueueType::Graphics) override;
    void Submit(ICommandList& commandList) override;
    void WaitIdle() override;

    BackendType GetBackendType() const override;
    const char* GetName() const override;

private:
    friend class VulkanCommandList;
    friend class VulkanRenderBackend;

    struct BufferResource
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
    };

    struct ImageResource
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        ImageDesc desc;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        bool owned = true;
    };

    struct SamplerResource
    {
        VkSampler sampler = VK_NULL_HANDLE;
    };

    struct ShaderModuleResource
    {
        VkShaderModule module = VK_NULL_HANDLE;
        ShaderStage stage = ShaderStage::None;
        std::string entryPoint = "main";
    };

    struct DescriptorSetLayoutResource
    {
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        DescriptorSetLayoutDesc desc;
    };

    struct DescriptorSetResource
    {
        VkDescriptorSet set = VK_NULL_HANDLE;
        DescriptorSetLayoutHandle layout;
    };

    struct PipelineResource
    {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        bool compute = false;
    };

    uint32_t AllocateHandle();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkCommandBuffer AllocateCommandBuffer();
    VkCommandBuffer BeginImmediateCommandBuffer();
    void EndImmediateCommandBuffer(VkCommandBuffer commandBuffer);
    VkRenderPass GetOrCreateRenderPass(const RenderTargetLayoutDesc& layout);
    VkFramebuffer CreateFramebuffer(const RenderPassBeginInfo& beginInfo, VkRenderPass renderPass);
    void TransitionImage(ImageHandle handle, VkImageLayout newLayout, VkImageAspectFlags aspectMask);
    void TransitionImage(ImageResource& image, VkImageLayout newLayout, VkImageAspectFlags aspectMask);

    const BufferResource* GetBuffer(BufferHandle handle) const;
    ImageResource* GetImage(ImageHandle handle);
    const ImageResource* GetImage(ImageHandle handle) const;
    const SamplerResource* GetSampler(SamplerHandle handle) const;
    const ShaderModuleResource* GetShaderModule(ShaderModuleHandle handle) const;
    const DescriptorSetLayoutResource* GetDescriptorSetLayout(DescriptorSetLayoutHandle handle) const;
    const DescriptorSetResource* GetDescriptorSet(DescriptorSetHandle handle) const;
    const PipelineResource* GetPipeline(PipelineHandle handle) const;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamily = 0;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VulkanCommandList m_CommandList;
    uint32_t m_NextHandle = 1;
    std::map<std::string, VkRenderPass> m_RenderPassCache;

    std::unordered_map<uint32_t, BufferResource> m_Buffers;
    std::unordered_map<uint32_t, ImageResource> m_Images;
    std::unordered_map<uint32_t, SamplerResource> m_Samplers;
    std::unordered_map<uint32_t, ShaderModuleResource> m_ShaderModules;
    std::unordered_map<uint32_t, DescriptorSetLayoutResource> m_DescriptorSetLayouts;
    std::unordered_map<uint32_t, DescriptorSetResource> m_DescriptorSets;
    std::unordered_map<uint32_t, PipelineResource> m_Pipelines;
};
}  // namespace rhi
#endif