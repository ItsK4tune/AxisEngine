#pragma once

#include <render/rhi/i_command_list.h>
#include <render/rhi/rhi_types.h>

namespace rhi
{
class IRenderDevice
{
public:
    virtual ~IRenderDevice() = default;

    virtual BufferHandle CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual void UpdateBuffer(BufferHandle handle, uint64_t offset, uint64_t size, const void* data) = 0;
    virtual void DestroyBuffer(BufferHandle handle) = 0;

    virtual ImageHandle CreateImage(const ImageDesc& desc, const void* initialData = nullptr) = 0;
    virtual void DestroyImage(ImageHandle handle) = 0;

    virtual SamplerHandle CreateSampler(const SamplerDesc& desc) = 0;
    virtual void DestroySampler(SamplerHandle handle) = 0;

    virtual ShaderModuleHandle CreateShaderModule(const ShaderModuleDesc& desc) = 0;
    virtual void DestroyShaderModule(ShaderModuleHandle handle) = 0;

    virtual DescriptorSetLayoutHandle CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) = 0;
    virtual void DestroyDescriptorSetLayout(DescriptorSetLayoutHandle handle) = 0;

    virtual DescriptorSetHandle CreateDescriptorSet(DescriptorSetLayoutHandle layout) = 0;
    virtual void UpdateDescriptorSet(DescriptorSetHandle set, const DescriptorUpdate* updates, uint32_t count) = 0;
    virtual void DestroyDescriptorSet(DescriptorSetHandle handle) = 0;

    virtual PipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;
    virtual PipelineHandle CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual void DestroyPipeline(PipelineHandle handle) = 0;

    virtual ICommandList& BeginCommandList(CommandQueueType queue = CommandQueueType::Graphics) = 0;
    virtual void Submit(ICommandList& commandList) = 0;
    virtual void WaitIdle() = 0;

    virtual BackendType GetBackendType() const = 0;
    virtual const char* GetName() const = 0;
};
}  // namespace rhi
