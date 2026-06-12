#pragma once
#include <render/rhi/i_command_list.h>
#if AXIS_HAS_VULKAN_BACKEND
#include <vulkan/vulkan.h>
#include <vector>

namespace rhi
{
class VulkanRenderDevice;
class VulkanCommandList final : public ICommandList
{
public:
    explicit VulkanCommandList(VulkanRenderDevice& device);

    void Begin() override;
    void End() override;
    void BeginRendering(const RenderPassBeginInfo& beginInfo) override;
    void EndRendering() override;
    void SetViewport(const Viewport& viewport) override;
    void SetScissor(const Rect2D& scissor) override;
    void BindPipeline(PipelineHandle pipeline) override;
    void BindDescriptorSet(uint32_t setIndex, DescriptorSetHandle descriptorSet) override;
    void BindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset = 0) override;
    void BindIndexBuffer(BufferHandle buffer, IndexType indexType, uint64_t offset = 0) override;
    void PushConstants(ShaderStage stages, const void* data, uint32_t size, uint32_t offset = 0) override;
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
              uint32_t firstInstance = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                     uint32_t firstInstance = 0) override;
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

    VkCommandBuffer GetNative() const
    {
        return m_CommandBuffer;
    }

private:
    VulkanRenderDevice& m_Device;
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    VkPipelineLayout m_CurrentPipelineLayout = VK_NULL_HANDLE;
    VkPipelineBindPoint m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkRenderPass m_CurrentRenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_CurrentFramebuffer = VK_NULL_HANDLE;
    std::vector<ImageHandle> m_CurrentColorAttachments;
    ImageHandle m_CurrentDepthAttachment;
};
}  // namespace rhi
#endif