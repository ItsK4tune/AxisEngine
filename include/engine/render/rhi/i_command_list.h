#pragma once

#include <render/rhi/rhi_types.h>

namespace rhi
{
class ICommandList
{
public:
    virtual ~ICommandList() = default;

    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void BeginRendering(const RenderPassBeginInfo& beginInfo) = 0;
    virtual void EndRendering() = 0;

    virtual void SetViewport(const Viewport& viewport) = 0;
    virtual void SetScissor(const Rect2D& scissor) = 0;

    virtual void BindPipeline(PipelineHandle pipeline) = 0;
    virtual void BindDescriptorSet(uint32_t setIndex, DescriptorSetHandle descriptorSet) = 0;
    virtual void BindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset = 0) = 0;
    virtual void BindIndexBuffer(BufferHandle buffer, IndexType indexType, uint64_t offset = 0) = 0;

    virtual void PushConstants(ShaderStage stages, const void* data, uint32_t size, uint32_t offset = 0) = 0;

    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
                      uint32_t firstInstance = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0,
                             int32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
    virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
};
}  // namespace rhi
