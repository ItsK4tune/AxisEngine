#include <render/strategy/vulkan/vulkan_command_list.h>
#if AXIS_HAS_VULKAN_BACKEND
#include <render/strategy/vulkan/vulkan_render_device.h>
#include <render/strategy/vulkan/vulkan_utils.h>
#include <algorithm>
#include <vector>
namespace rhi
{
VulkanCommandList::VulkanCommandList(VulkanRenderDevice& device) : m_Device(device)
{
}

void VulkanCommandList::Begin()
{
    if (m_CommandBuffer == VK_NULL_HANDLE)
        m_CommandBuffer = m_Device.AllocateCommandBuffer();
    else
        vkResetCommandBuffer(m_CommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    Check(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo), "vkBeginCommandBuffer");
    m_CurrentPipelineLayout = VK_NULL_HANDLE;
    m_CurrentPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void VulkanCommandList::End()
{
    if (m_CommandBuffer)
        Check(vkEndCommandBuffer(m_CommandBuffer), "vkEndCommandBuffer");
}

void VulkanCommandList::BeginRendering(const RenderPassBeginInfo& beginInfo)
{
    RenderTargetLayoutDesc layout;
    layout.colorFormats.reserve(beginInfo.colorAttachments.size());
    m_CurrentColorAttachments.clear();
    for (const auto& attachment : beginInfo.colorAttachments)
    {
        layout.colorFormats.push_back(attachment.format);
        m_Device.TransitionImage(attachment.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        m_CurrentColorAttachments.push_back(attachment.image);
    }
    if (beginInfo.hasDepthStencilAttachment)
    {
        layout.depthStencilFormat = beginInfo.depthStencilAttachment.format;
        m_Device.TransitionImage(beginInfo.depthStencilAttachment.image,
                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                 GetAspectMask(beginInfo.depthStencilAttachment.format));
        m_CurrentDepthAttachment = beginInfo.depthStencilAttachment.image;
    }
    else
    {
        m_CurrentDepthAttachment = {};
    }

    m_CurrentRenderPass = m_Device.GetOrCreateRenderPass(layout);
    m_CurrentFramebuffer = m_Device.CreateFramebuffer(beginInfo, m_CurrentRenderPass);
    if (!m_CurrentRenderPass || !m_CurrentFramebuffer)
        return;

    VkRenderPassBeginInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = m_CurrentRenderPass;
    renderPassInfo.framebuffer = m_CurrentFramebuffer;
    renderPassInfo.renderArea.offset = {beginInfo.renderArea.x, beginInfo.renderArea.y};
    renderPassInfo.renderArea.extent = {beginInfo.renderArea.width, beginInfo.renderArea.height};
    vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    std::vector<VkClearAttachment> clears;
    VkClearRect clearRect{};
    clearRect.rect = renderPassInfo.renderArea;
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount = 1;

    for (uint32_t i = 0; i < beginInfo.colorAttachments.size(); ++i)
    {
        const auto& attachment = beginInfo.colorAttachments[i];
        if (attachment.loadOp != LoadOp::Clear)
            continue;
        VkClearAttachment clear{};
        clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clear.colorAttachment = i;
        clear.clearValue.color = {
            {attachment.clearColor.r, attachment.clearColor.g, attachment.clearColor.b, attachment.clearColor.a}};
        clears.push_back(clear);
    }

    if (beginInfo.hasDepthStencilAttachment && beginInfo.depthStencilAttachment.depthLoadOp == LoadOp::Clear)
    {
        VkClearAttachment clear{};
        clear.aspectMask = GetAspectMask(beginInfo.depthStencilAttachment.format);
        clear.clearValue.depthStencil = {beginInfo.depthStencilAttachment.clearValue.depth,
                                         beginInfo.depthStencilAttachment.clearValue.stencil};
        clears.push_back(clear);
    }

    if (!clears.empty())
        vkCmdClearAttachments(m_CommandBuffer, static_cast<uint32_t>(clears.size()), clears.data(), 1, &clearRect);
}

void VulkanCommandList::EndRendering()
{
    if (m_CurrentRenderPass)
        vkCmdEndRenderPass(m_CommandBuffer);

    for (auto image : m_CurrentColorAttachments)
    {
        if (auto* resource = m_Device.GetImage(image))
        {
            if (HasFlag(resource->desc.usage, ImageUsage::Present))
                m_Device.TransitionImage(*resource, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);
            else
                m_Device.TransitionImage(*resource, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                         VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    if (m_CurrentDepthAttachment)
    {
        if (auto* resource = m_Device.GetImage(m_CurrentDepthAttachment))
            m_Device.TransitionImage(*resource, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                                     GetAspectMask(resource->desc.format));
    }

    if (m_CurrentFramebuffer)
    {
        vkDestroyFramebuffer(m_Device.m_Device, m_CurrentFramebuffer, nullptr);
        m_CurrentFramebuffer = VK_NULL_HANDLE;
    }
    m_CurrentRenderPass = VK_NULL_HANDLE;
    m_CurrentColorAttachments.clear();
    m_CurrentDepthAttachment = {};
}

void VulkanCommandList::SetViewport(const Viewport& viewport)
{
    VkViewport vkViewport{viewport.x,      viewport.y,        viewport.width,
                          viewport.height, viewport.minDepth, viewport.maxDepth};
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &vkViewport);
}

void VulkanCommandList::SetScissor(const Rect2D& scissor)
{
    VkRect2D rect{{scissor.x, scissor.y}, {scissor.width, scissor.height}};
    vkCmdSetScissor(m_CommandBuffer, 0, 1, &rect);
}

void VulkanCommandList::BindPipeline(PipelineHandle pipeline)
{
    const auto* resource = m_Device.GetPipeline(pipeline);
    if (!resource)
        return;
    m_CurrentPipelineBindPoint = resource->compute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
    vkCmdBindPipeline(m_CommandBuffer, m_CurrentPipelineBindPoint, resource->pipeline);
    m_CurrentPipelineLayout = resource->layout;
}

void VulkanCommandList::BindDescriptorSet(uint32_t setIndex, DescriptorSetHandle descriptorSet)
{
    const auto* resource = m_Device.GetDescriptorSet(descriptorSet);
    if (!resource || !m_CurrentPipelineLayout)
        return;
    vkCmdBindDescriptorSets(m_CommandBuffer, m_CurrentPipelineBindPoint, m_CurrentPipelineLayout, setIndex, 1,
                            &resource->set, 0, nullptr);
}

void VulkanCommandList::BindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset)
{
    const auto* resource = m_Device.GetBuffer(buffer);
    if (!resource)
        return;
    VkBuffer vkBuffer = resource->buffer;
    VkDeviceSize vkOffset = offset;
    vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, &vkBuffer, &vkOffset);
}

void VulkanCommandList::BindIndexBuffer(BufferHandle buffer, IndexType indexType, uint64_t offset)
{
    const auto* resource = m_Device.GetBuffer(buffer);
    if (!resource)
        return;
    vkCmdBindIndexBuffer(m_CommandBuffer, resource->buffer, offset,
                         indexType == IndexType::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
}

void VulkanCommandList::PushConstants(ShaderStage stages, const void* data, uint32_t size, uint32_t offset)
{
    if (m_CurrentPipelineLayout && data && size > 0)
        vkCmdPushConstants(m_CommandBuffer, m_CurrentPipelineLayout, ToVkShaderStages(stages), offset, size, data);
}

void VulkanCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                    int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    vkCmdDispatch(m_CommandBuffer, groupCountX, groupCountY, groupCountZ);
}

}  // namespace rhi
#endif