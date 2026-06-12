#pragma once

#include <render/rhi/i_command_list.h>
#include <render/rhi/i_render_device.h>
#include <render/rhi/i_swapchain.h>
#include <render/rhi/rhi_types.h>

namespace rhi
{
using RHIDevice = IRenderDevice;
using RHICommandBuffer = ICommandList;
using RHISwapchain = ISwapchain;

class RHIResource
{
public:
    virtual ~RHIResource() = default;
    virtual const char* GetDebugName() const
    {
        return "";
    }
};

class RHIBuffer : public RHIResource
{
public:
    virtual BufferHandle GetHandle() const = 0;
    virtual uint64_t GetSize() const = 0;
};

class RHITexture : public RHIResource
{
public:
    virtual ImageHandle GetHandle() const = 0;
    virtual Extent2D GetExtent() const = 0;
};

class RHIShader : public RHIResource
{
public:
    virtual ShaderModuleHandle GetHandle() const = 0;
};

class RHIPipelineLayout : public RHIResource
{
public:
    virtual DescriptorSetLayoutHandle GetHandle() const = 0;
};

class RHIPipeline : public RHIResource
{
public:
    virtual PipelineHandle GetHandle() const = 0;
};

class RHIGraphicsPipeline : public RHIPipeline
{
};

class RHIComputePipeline : public RHIPipeline
{
};

class RHIQueue : public RHIResource
{
public:
    virtual CommandQueueType GetQueueType() const = 0;
};

class RHIRenderPass : public RHIResource
{
};

class RHIFence : public RHIResource
{
public:
    virtual bool Wait(uint64_t timeoutNs) = 0;
    virtual void Reset() = 0;
    virtual bool IsSignaled() const = 0;
};

class RHISemaphore : public RHIResource
{
public:
    virtual void Signal() = 0;
    virtual bool IsSignaled() const = 0;
};

class RHIBarrier : public RHIResource
{
public:
    virtual void Apply(ICommandList& commandList) = 0;
};
}  // namespace rhi
