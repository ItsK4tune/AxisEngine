#include <render/strategy/directx/directx12_command_list.h>
#if AXIS_HAS_DIRECTX_BACKEND
#include <render/strategy/directx/directx12_render_device.h>
#include <render/strategy/directx/directx12_utils.h>
#include <algorithm>
namespace rhi
{
DirectX12CommandList::DirectX12CommandList(DirectX12RenderDevice& device) : m_Device(device)
{
}

void DirectX12CommandList::Begin()
{
    Check(m_CommandAllocator->Reset(), "ID3D12CommandAllocator::Reset");
    Check(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr), "ID3D12GraphicsCommandList::Reset");
    m_CurrentPipeline = {};
    m_CurrentPipelineIsCompute = false;
    m_CurrentPushConstantsRootIndex = UINT32_MAX;
    m_CurrentColorAttachments.clear();
    m_CurrentDepthAttachment = {};
}

void DirectX12CommandList::End()
{
    Check(m_CommandList->Close(), "ID3D12GraphicsCommandList::Close");
}

void DirectX12CommandList::BeginRendering(const RenderPassBeginInfo& beginInfo)
{
    m_CurrentColorAttachments.clear();
    m_CurrentDepthAttachment = {};

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs;
    rtvs.reserve(beginInfo.colorAttachments.size());
    for (const auto& attachment : beginInfo.colorAttachments)
    {
        auto* image = m_Device.GetImage(attachment.image);
        if (!image)
            continue;
        m_Device.TransitionResource(m_CommandList.Get(), *image, D3D12_RESOURCE_STATE_RENDER_TARGET);
        rtvs.push_back(image->rtv);
        m_CurrentColorAttachments.push_back(attachment.image);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
    D3D12_CPU_DESCRIPTOR_HANDLE* dsvPtr = nullptr;
    if (beginInfo.hasDepthStencilAttachment)
    {
        auto* depth = m_Device.GetImage(beginInfo.depthStencilAttachment.image);
        if (depth)
        {
            m_Device.TransitionResource(m_CommandList.Get(), *depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            dsv = depth->dsv;
            dsvPtr = &dsv;
            m_CurrentDepthAttachment = beginInfo.depthStencilAttachment.image;
        }
    }

    m_CommandList->OMSetRenderTargets(static_cast<UINT>(rtvs.size()), rtvs.empty() ? nullptr : rtvs.data(), FALSE,
                                      dsvPtr);

    SetScissor(beginInfo.renderArea);
    SetViewport(Viewport{static_cast<float>(beginInfo.renderArea.x), static_cast<float>(beginInfo.renderArea.y),
                         static_cast<float>(beginInfo.renderArea.width),
                         static_cast<float>(beginInfo.renderArea.height), 0.0f, 1.0f});

    for (uint32_t i = 0; i < beginInfo.colorAttachments.size() && i < rtvs.size(); ++i)
    {
        const auto& attachment = beginInfo.colorAttachments[i];
        if (attachment.loadOp != LoadOp::Clear)
            continue;
        const FLOAT color[4] = {attachment.clearColor.r, attachment.clearColor.g, attachment.clearColor.b,
                                attachment.clearColor.a};
        m_CommandList->ClearRenderTargetView(rtvs[i], color, 0, nullptr);
    }

    if (dsvPtr && beginInfo.depthStencilAttachment.depthLoadOp == LoadOp::Clear)
    {
        const auto clear = beginInfo.depthStencilAttachment.clearValue;
        D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
        if (beginInfo.depthStencilAttachment.format == Format::D24S8)
            flags |= D3D12_CLEAR_FLAG_STENCIL;
        m_CommandList->ClearDepthStencilView(*dsvPtr, flags, clear.depth,
                                             static_cast<UINT8>(clear.stencil), 0, nullptr);
    }
}

void DirectX12CommandList::EndRendering()
{
    for (auto imageHandle : m_CurrentColorAttachments)
    {
        auto* image = m_Device.GetImage(imageHandle);
        if (!image)
            continue;
        if (HasFlag(image->desc.usage, ImageUsage::Present))
            m_Device.TransitionResource(m_CommandList.Get(), *image, D3D12_RESOURCE_STATE_PRESENT);
        else if (HasFlag(image->desc.usage, ImageUsage::Sampled))
            m_Device.TransitionResource(
                m_CommandList.Get(), *image,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        else
            m_Device.TransitionResource(m_CommandList.Get(), *image, D3D12_RESOURCE_STATE_COMMON);
    }

    if (auto* depth = m_Device.GetImage(m_CurrentDepthAttachment))
        m_Device.TransitionResource(m_CommandList.Get(), *depth, D3D12_RESOURCE_STATE_DEPTH_READ);

    m_CurrentColorAttachments.clear();
    m_CurrentDepthAttachment = {};
}

void DirectX12CommandList::SetViewport(const Viewport& viewport)
{
    D3D12_VIEWPORT d3dViewport{viewport.x,      viewport.y,        viewport.width,
                               viewport.height, viewport.minDepth, viewport.maxDepth};
    m_CommandList->RSSetViewports(1, &d3dViewport);
}

void DirectX12CommandList::SetScissor(const Rect2D& scissor)
{
    D3D12_RECT rect{scissor.x, scissor.y, scissor.x + static_cast<LONG>(scissor.width),
                    scissor.y + static_cast<LONG>(scissor.height)};
    m_CommandList->RSSetScissorRects(1, &rect);
}

void DirectX12CommandList::BindPipeline(PipelineHandle pipeline)
{
    const auto* resource = m_Device.GetPipeline(pipeline);
    if (!resource)
        return;

    m_CurrentPipeline = pipeline;
    m_CurrentPipelineIsCompute = resource->compute;
    m_CurrentPushConstantsRootIndex = resource->pushConstantsRootIndex;
    m_CommandList->SetPipelineState(resource->pipeline.Get());
    if (resource->compute)
        m_CommandList->SetComputeRootSignature(resource->rootSignature.Get());
    else
    {
        m_CommandList->SetGraphicsRootSignature(resource->rootSignature.Get());
        m_CommandList->IASetPrimitiveTopology(ToD3DTopology(resource->topology));
        for (const auto& [binding, bound] : m_BoundVertexBuffers) ApplyVertexBuffer(binding, bound);
    }
}

void DirectX12CommandList::BindDescriptorSet(uint32_t setIndex, DescriptorSetHandle descriptorSet)
{
    ApplyDescriptorSet(setIndex, descriptorSet);
}

void DirectX12CommandList::BindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset)
{
    BoundVertexBuffer bound{buffer, offset};
    m_BoundVertexBuffers[binding] = bound;
    ApplyVertexBuffer(binding, bound);
}

void DirectX12CommandList::BindIndexBuffer(BufferHandle buffer, IndexType indexType, uint64_t offset)
{
    const auto* resource = m_Device.GetBuffer(buffer);
    if (!resource)
        return;

    D3D12_INDEX_BUFFER_VIEW view{};
    view.BufferLocation = resource->resource->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(resource->size - offset);
    view.Format = indexType == IndexType::UInt16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    m_CommandList->IASetIndexBuffer(&view);
}

void DirectX12CommandList::PushConstants(ShaderStage stages, const void* data, uint32_t size, uint32_t offset)
{
    (void)stages;
    if (m_CurrentPushConstantsRootIndex == UINT32_MAX || !data || size == 0)
        return;

    const UINT count = static_cast<UINT>((size + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    const UINT destOffset = offset / sizeof(uint32_t);
    if (m_CurrentPipelineIsCompute)
        m_CommandList->SetComputeRoot32BitConstants(m_CurrentPushConstantsRootIndex, count, data, destOffset);
    else
        m_CommandList->SetGraphicsRoot32BitConstants(m_CurrentPushConstantsRootIndex, count, data, destOffset);
}

void DirectX12CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                uint32_t firstInstance)
{
    m_CommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void DirectX12CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                       int32_t vertexOffset, uint32_t firstInstance)
{
    m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void DirectX12CommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    m_CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void DirectX12CommandList::ApplyVertexBuffer(uint32_t binding, const BoundVertexBuffer& bound)
{
    const auto* pipeline = m_Device.GetPipeline(m_CurrentPipeline);
    const auto* buffer = m_Device.GetBuffer(bound.buffer);
    if (!pipeline || pipeline->compute || !buffer)
        return;

    auto strideIt = pipeline->vertexStrides.find(binding);
    if (strideIt == pipeline->vertexStrides.end())
        return;

    D3D12_VERTEX_BUFFER_VIEW view{};
    view.BufferLocation = buffer->resource->GetGPUVirtualAddress() + bound.offset;
    view.SizeInBytes = static_cast<UINT>(buffer->size - bound.offset);
    view.StrideInBytes = strideIt->second;
    m_CommandList->IASetVertexBuffers(binding, 1, &view);
}

void DirectX12CommandList::ApplyDescriptorSet(uint32_t setIndex, DescriptorSetHandle descriptorSet)
{
    const auto* pipeline = m_Device.GetPipeline(m_CurrentPipeline);
    const auto* set = m_Device.GetDescriptorSet(descriptorSet);
    if (!pipeline || !set)
        return;

    ID3D12DescriptorHeap* heaps[] = {m_Device.m_CbvSrvUavHeap.Get(), m_Device.m_SamplerHeap.Get()};
    m_CommandList->SetDescriptorHeaps(2, heaps);

    for (const auto& rootBinding : pipeline->rootBindings)
    {
        if (rootBinding.set != setIndex)
            continue;
        auto slotIt = set->slots.find(rootBinding.binding);
        if (slotIt == set->slots.end())
            continue;

        D3D12_GPU_DESCRIPTOR_HANDLE gpu = rootBinding.sampler ? m_Device.GetSamplerGpu(slotIt->second.samplerIndex)
                                                              : m_Device.GetCbvSrvUavGpu(slotIt->second.cbvSrvUavIndex);
        if (gpu.ptr == 0)
            continue;

        if (pipeline->compute)
            m_CommandList->SetComputeRootDescriptorTable(rootBinding.rootIndex, gpu);
        else
            m_CommandList->SetGraphicsRootDescriptorTable(rootBinding.rootIndex, gpu);
    }
}

void DirectX12CommandList::TransitionImage(ImageHandle handle, D3D12_RESOURCE_STATES newState)
{
    if (auto* image = m_Device.GetImage(handle))
        m_Device.TransitionResource(m_CommandList.Get(), *image, newState);
}

}  // namespace rhi
#endif
