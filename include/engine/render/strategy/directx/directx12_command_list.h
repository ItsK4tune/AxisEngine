#pragma once
#include <render/rhi/i_command_list.h>
#include <render/strategy/directx/directx12_common.h>
#if AXIS_HAS_DIRECTX_BACKEND
#include <unordered_map>
#include <vector>

namespace rhi
{
class DirectX12RenderDevice;
class DirectX12CommandList final : public ICommandList
{
public:
    explicit DirectX12CommandList(DirectX12RenderDevice& device);

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

    ID3D12GraphicsCommandList* GetNative() const
    {
        return m_CommandList.Get();
    }

private:
    friend class DirectX12RenderDevice;

    struct BoundVertexBuffer
    {
        BufferHandle buffer;
        uint64_t offset = 0;
    };

    void ApplyVertexBuffer(uint32_t binding, const BoundVertexBuffer& bound);
    void ApplyDescriptorSet(uint32_t setIndex, DescriptorSetHandle descriptorSet);
    void TransitionImage(ImageHandle handle, D3D12_RESOURCE_STATES newState);

    DirectX12RenderDevice& m_Device;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
    PipelineHandle m_CurrentPipeline;
    bool m_CurrentPipelineIsCompute = false;
    uint32_t m_CurrentPushConstantsRootIndex = UINT32_MAX;
    std::unordered_map<uint32_t, BoundVertexBuffer> m_BoundVertexBuffers;
    std::vector<ImageHandle> m_CurrentColorAttachments;
    ImageHandle m_CurrentDepthAttachment;
};
}  // namespace rhi
#endif