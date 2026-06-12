#include <render/strategy/directx/directx12_render_device.h>
#if AXIS_HAS_DIRECTX_BACKEND
#include <render/strategy/directx/directx12_utils.h>
#include <algorithm>
#include <cstring>
namespace rhi
{
DirectX12RenderDevice::DirectX12RenderDevice() : m_CommandList(*this)
{
}

DirectX12RenderDevice::~DirectX12RenderDevice()
{
    Shutdown();
}

bool DirectX12RenderDevice::Initialize(ID3D12Device* device, ID3D12CommandQueue* graphicsQueue)
{
    m_Device = device;
    m_GraphicsQueue = graphicsQueue;

    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavDesc{};
    cbvSrvUavDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvUavDesc.NumDescriptors = kCbvSrvUavHeapCapacity;
    cbvSrvUavDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (!Check(m_Device->CreateDescriptorHeap(&cbvSrvUavDesc, IID_PPV_ARGS(&m_CbvSrvUavHeap)),
               "CreateDescriptorHeap(CBV/SRV/UAV)"))
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC samplerDesc{};
    samplerDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerDesc.NumDescriptors = kSamplerHeapCapacity;
    samplerDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (!Check(m_Device->CreateDescriptorHeap(&samplerDesc, IID_PPV_ARGS(&m_SamplerHeap)),
               "CreateDescriptorHeap(Sampler)"))
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = kRtvHeapCapacity;
    if (!Check(m_Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_RtvHeap)), "CreateDescriptorHeap(RTV)"))
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc{};
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.NumDescriptors = kDsvHeapCapacity;
    if (!Check(m_Device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&m_DsvHeap)), "CreateDescriptorHeap(DSV)"))
        return false;

    m_CbvSrvUavDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_SamplerDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_DsvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    if (!Check(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                IID_PPV_ARGS(&m_CommandList.m_CommandAllocator)),
               "CreateCommandAllocator"))
        return false;
    if (!Check(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandList.m_CommandAllocator.Get(),
                                           nullptr, IID_PPV_ARGS(&m_CommandList.m_CommandList)),
               "CreateCommandList"))
        return false;
    m_CommandList.m_CommandList->Close();

    return Check(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)), "CreateFence");
}

void DirectX12RenderDevice::Shutdown()
{
    if (!m_Device)
        return;

    WaitIdle();
    m_Pipelines.clear();
    m_DescriptorSets.clear();
    m_DescriptorSetLayouts.clear();
    m_ShaderModules.clear();
    m_Samplers.clear();
    m_Images.clear();
    m_Buffers.clear();
    m_CommandList.m_CommandList.Reset();
    m_CommandList.m_CommandAllocator.Reset();
    m_Fence.Reset();
    m_CbvSrvUavHeap.Reset();
    m_SamplerHeap.Reset();
    m_RtvHeap.Reset();
    m_DsvHeap.Reset();
    m_GraphicsQueue.Reset();
    m_Device.Reset();
}

BufferHandle DirectX12RenderDevice::CreateBuffer(const BufferDesc& desc, const void* initialData)
{
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = ToHeapType(desc.memoryUsage);

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = HasFlag(desc.usage, BufferUsage::Uniform) ? std::max<uint64_t>(AlignUp(desc.size, 256), 256)
                                                                   : std::max<uint64_t>(desc.size, 1);
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (HasFlag(desc.usage, BufferUsage::Storage))
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_RESOURCE_STATES initialState = InitialBufferState(desc.memoryUsage);
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    if (!Check(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
                                                 nullptr, IID_PPV_ARGS(&resource)),
               "CreateCommittedResource(Buffer)"))
        return {};

    const uint32_t id = AllocateHandle();
    m_Buffers[id] = BufferResource{resource, desc.size, desc.usage, desc.memoryUsage, initialState};
    if (initialData)
        UpdateBuffer(BufferHandle{id}, 0, desc.size, initialData);
    return BufferHandle{id};
}

void DirectX12RenderDevice::UpdateBuffer(BufferHandle handle, uint64_t offset, uint64_t size, const void* data)
{
    auto* buffer = GetBuffer(handle);
    if (!buffer || !data || offset + size > buffer->size)
        return;

    if (buffer->memoryUsage != MemoryUsage::GpuOnly)
    {
        void* mapped = nullptr;
        D3D12_RANGE readRange{0, 0};
        if (Check(buffer->resource->Map(0, &readRange, &mapped), "ID3D12Resource::Map(Buffer)"))
        {
            std::memcpy(static_cast<uint8_t*>(mapped) + offset, data, static_cast<size_t>(size));
            D3D12_RANGE writeRange{static_cast<SIZE_T>(offset), static_cast<SIZE_T>(offset + size)};
            buffer->resource->Unmap(0, &writeRange);
        }
        return;
    }

    BufferDesc uploadDesc;
    uploadDesc.size = size;
    uploadDesc.usage = BufferUsage::TransferSrc;
    uploadDesc.memoryUsage = MemoryUsage::CpuToGpu;
    uploadDesc.debugName = "DirectX12BufferUpload";
    BufferHandle uploadHandle = CreateBuffer(uploadDesc, data);
    const auto* upload = GetBuffer(uploadHandle);
    if (!upload)
        return;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    if (!Check(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
               "CreateCommandAllocator(Upload)") ||
        !Check(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                           IID_PPV_ARGS(&commandList)),
               "CreateCommandList(Upload)"))
    {
        DestroyBuffer(uploadHandle);
        return;
    }

    const D3D12_RESOURCE_STATES oldState = buffer->state;
    if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = buffer->resource.Get();
        barrier.Transition.StateBefore = oldState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }
    commandList->CopyBufferRegion(buffer->resource.Get(), offset, upload->resource.Get(), 0, size);
    if (oldState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = buffer->resource.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = oldState;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }
    commandList->Close();
    ID3D12CommandList* lists[] = {commandList.Get()};
    ExecuteImmediate(lists[0]);
    buffer->state = oldState;
    DestroyBuffer(uploadHandle);
}

void DirectX12RenderDevice::DestroyBuffer(BufferHandle handle)
{
    m_Buffers.erase(handle.id);
}

ImageHandle DirectX12RenderDevice::CreateImage(const ImageDesc& desc, const void* initialData)
{
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = ToResourceDimension(desc.dimension);
    resourceDesc.Alignment = 0;
    resourceDesc.Width = desc.width;
    resourceDesc.Height = desc.height;
    resourceDesc.DepthOrArraySize = static_cast<UINT16>(
        desc.dimension == ImageDimension::Image3D ? desc.depth
                                                  : (desc.dimension == ImageDimension::Cube ? 6 : desc.arrayLayers));
    resourceDesc.MipLevels = static_cast<UINT16>(desc.mipLevels);
    resourceDesc.Format = ToResourceDxgiFormat(desc.format, desc.usage);
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    if (HasFlag(desc.usage, ImageUsage::ColorAttachment))
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (HasFlag(desc.usage, ImageUsage::DepthStencilAttachment))
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (HasFlag(desc.usage, ImageUsage::Storage))
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE clearValue{};
    D3D12_CLEAR_VALUE* clearPtr = nullptr;
    if (HasFlag(desc.usage, ImageUsage::ColorAttachment))
    {
        clearValue.Format = ToDxgiFormat(desc.format);
        clearPtr = &clearValue;
    }
    else if (HasFlag(desc.usage, ImageUsage::DepthStencilAttachment))
    {
        clearValue.Format = ToDepthStencilViewFormat(desc.format);
        clearValue.DepthStencil.Depth = 1.0f;
        clearPtr = &clearValue;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_RESOURCE_STATES initialState = initialData ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON;
    if (!Check(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialState,
                                                 clearPtr, IID_PPV_ARGS(&resource)),
               "CreateCommittedResource(Image)"))
        return {};

    const uint32_t id = AllocateHandle();
    ImageResource imageResource;
    imageResource.resource = resource;
    imageResource.desc = desc;
    imageResource.state = initialState;
    imageResource.owned = true;

    if (HasFlag(desc.usage, ImageUsage::ColorAttachment))
    {
        const uint32_t index = AllocateRtv();
        imageResource.rtv = GetRtvCpu(index);
        m_Device->CreateRenderTargetView(resource.Get(), nullptr, imageResource.rtv);
    }
    if (HasFlag(desc.usage, ImageUsage::DepthStencilAttachment))
    {
        const uint32_t index = AllocateDsv();
        imageResource.dsv = GetDsvCpu(index);
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv{};
        dsv.Format = ToDepthStencilViewFormat(desc.format);
        dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        m_Device->CreateDepthStencilView(resource.Get(), &dsv, imageResource.dsv);
    }
    if (HasFlag(desc.usage, ImageUsage::Sampled))
    {
        const uint32_t index = AllocateCbvSrvUav();
        imageResource.srv = GetCbvSrvUavCpu(index);
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format = ToShaderVisibleDxgiFormat(desc.format);
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (desc.dimension == ImageDimension::Image3D)
        {
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            srv.Texture3D.MipLevels = desc.mipLevels;
        }
        else if (desc.dimension == ImageDimension::Cube)
        {
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srv.TextureCube.MipLevels = desc.mipLevels;
        }
        else
        {
            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv.Texture2D.MipLevels = desc.mipLevels;
        }
        m_Device->CreateShaderResourceView(resource.Get(), &srv, imageResource.srv);
    }
    if (HasFlag(desc.usage, ImageUsage::Storage))
    {
        const uint32_t index = AllocateCbvSrvUav();
        imageResource.uav = GetCbvSrvUavCpu(index);
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
        uav.Format = ToShaderVisibleDxgiFormat(desc.format);
        uav.ViewDimension =
            desc.dimension == ImageDimension::Image3D ? D3D12_UAV_DIMENSION_TEXTURE3D : D3D12_UAV_DIMENSION_TEXTURE2D;
        m_Device->CreateUnorderedAccessView(resource.Get(), nullptr, &uav, imageResource.uav);
    }

    m_Images[id] = imageResource;
    if (initialData)
        UploadTextureData(m_Images[id], initialData);
    return ImageHandle{id};
}

void DirectX12RenderDevice::DestroyImage(ImageHandle handle)
{
    m_Images.erase(handle.id);
}

SamplerHandle DirectX12RenderDevice::CreateSampler(const SamplerDesc& desc)
{
    const uint32_t index = AllocateSampler();
    D3D12_SAMPLER_DESC sampler{};
    sampler.Filter = ToD3DFilter(desc);
    sampler.AddressU = ToD3DAddressMode(desc.addressU);
    sampler.AddressV = ToD3DAddressMode(desc.addressV);
    sampler.AddressW = ToD3DAddressMode(desc.addressW);
    sampler.MaxAnisotropy = static_cast<UINT>(std::max(1.0f, desc.maxAnisotropy));
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    const auto cpu = GetSamplerCpu(index);
    m_Device->CreateSampler(&sampler, cpu);

    const uint32_t id = AllocateHandle();
    m_Samplers[id] = SamplerResource{cpu, GetSamplerGpu(index)};
    return SamplerHandle{id};
}

void DirectX12RenderDevice::DestroySampler(SamplerHandle handle)
{
    m_Samplers.erase(handle.id);
}

ShaderModuleHandle DirectX12RenderDevice::CreateShaderModule(const ShaderModuleDesc& desc)
{
    if (desc.sourceType != ShaderSourceType::DxilBinary || !desc.data || desc.size == 0)
    {
        LOGGER_ERROR("DirectX12RHI") << "DirectX 12 shader modules require DXIL binary input.";
        return {};
    }

    ShaderModuleResource resource;
    resource.stage = desc.stage;
    resource.bytecode.resize(static_cast<size_t>(desc.size));
    std::memcpy(resource.bytecode.data(), desc.data, static_cast<size_t>(desc.size));

    const uint32_t id = AllocateHandle();
    m_ShaderModules[id] = std::move(resource);
    return ShaderModuleHandle{id};
}

void DirectX12RenderDevice::DestroyShaderModule(ShaderModuleHandle handle)
{
    m_ShaderModules.erase(handle.id);
}

DescriptorSetLayoutHandle DirectX12RenderDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc)
{
    const uint32_t id = AllocateHandle();
    m_DescriptorSetLayouts[id] = DescriptorSetLayoutResource{desc};
    return DescriptorSetLayoutHandle{id};
}

void DirectX12RenderDevice::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle handle)
{
    m_DescriptorSetLayouts.erase(handle.id);
}

DescriptorSetHandle DirectX12RenderDevice::CreateDescriptorSet(DescriptorSetLayoutHandle layout)
{
    const auto* layoutResource = GetDescriptorSetLayout(layout);
    if (!layoutResource)
        return {};

    DescriptorSetResource setResource;
    setResource.layout = layout;
    for (const auto& binding : layoutResource->desc.bindings)
    {
        DescriptorSlot slot;
        slot.count = std::max(binding.count, 1u);
        slot.type = binding.type;
        if (binding.type == DescriptorType::Sampler)
        {
            slot.samplerIndex = AllocateSampler(slot.count);
        }
        else if (binding.type == DescriptorType::CombinedImageSampler)
        {
            slot.cbvSrvUavIndex = AllocateCbvSrvUav(slot.count);
            slot.samplerIndex = AllocateSampler(slot.count);
        }
        else
        {
            slot.cbvSrvUavIndex = AllocateCbvSrvUav(slot.count);
        }
        setResource.slots[binding.binding] = slot;
    }

    const uint32_t id = AllocateHandle();
    m_DescriptorSets[id] = std::move(setResource);
    return DescriptorSetHandle{id};
}

void DirectX12RenderDevice::UpdateDescriptorSet(DescriptorSetHandle set, const DescriptorUpdate* updates,
                                                uint32_t count)
{
    auto* setResource = GetDescriptorSet(set);
    if (!setResource || !updates)
        return;

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto& update = updates[i];
        auto slotIt = setResource->slots.find(update.binding);
        if (slotIt == setResource->slots.end() || update.arrayElement >= slotIt->second.count)
            continue;
        const auto& slot = slotIt->second;

        if (update.type == DescriptorType::UniformBuffer)
        {
            const auto* buffer = GetBuffer(update.buffer);
            if (!buffer)
                continue;
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
            cbv.BufferLocation = buffer->resource->GetGPUVirtualAddress() + update.bufferOffset;
            cbv.SizeInBytes = static_cast<UINT>(AlignUp(update.bufferRange ? update.bufferRange : buffer->size, 256));
            m_Device->CreateConstantBufferView(&cbv, GetCbvSrvUavCpu(slot.cbvSrvUavIndex + update.arrayElement));
        }
        else if (update.type == DescriptorType::StorageBuffer)
        {
            const auto* buffer = GetBuffer(update.buffer);
            if (!buffer)
                continue;
            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
            uav.Format = DXGI_FORMAT_R32_TYPELESS;
            uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uav.Buffer.FirstElement = update.bufferOffset / sizeof(uint32_t);
            uav.Buffer.NumElements =
                static_cast<UINT>((update.bufferRange ? update.bufferRange : buffer->size) / sizeof(uint32_t));
            uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
            m_Device->CreateUnorderedAccessView(buffer->resource.Get(), nullptr, &uav,
                                                GetCbvSrvUavCpu(slot.cbvSrvUavIndex + update.arrayElement));
        }
        else if (update.type == DescriptorType::SampledImage || update.type == DescriptorType::CombinedImageSampler)
        {
            const auto* image = GetImage(update.image);
            if (image && image->srv.ptr != 0)
            {
                m_Device->CopyDescriptorsSimple(1, GetCbvSrvUavCpu(slot.cbvSrvUavIndex + update.arrayElement),
                                                image->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
            const auto* sampler = GetSampler(update.sampler);
            if (sampler && slot.samplerIndex != UINT32_MAX)
            {
                m_Device->CopyDescriptorsSimple(1, GetSamplerCpu(slot.samplerIndex + update.arrayElement), sampler->cpu,
                                                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            }
        }
        else if (update.type == DescriptorType::StorageImage)
        {
            const auto* image = GetImage(update.image);
            if (image && image->uav.ptr != 0)
            {
                m_Device->CopyDescriptorsSimple(1, GetCbvSrvUavCpu(slot.cbvSrvUavIndex + update.arrayElement),
                                                image->uav, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            }
        }
        else if (update.type == DescriptorType::Sampler)
        {
            const auto* sampler = GetSampler(update.sampler);
            if (sampler)
            {
                m_Device->CopyDescriptorsSimple(1, GetSamplerCpu(slot.samplerIndex + update.arrayElement), sampler->cpu,
                                                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            }
        }

        auto existingIt = std::find_if(
            setResource->updates.begin(), setResource->updates.end(), [&](const DescriptorUpdate& existing) {
                return existing.binding == update.binding && existing.arrayElement == update.arrayElement;
            });
        if (existingIt == setResource->updates.end())
            setResource->updates.push_back(update);
        else
            *existingIt = update;
    }
}

void DirectX12RenderDevice::DestroyDescriptorSet(DescriptorSetHandle handle)
{
    m_DescriptorSets.erase(handle.id);
}

PipelineHandle DirectX12RenderDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
    const auto* vertexShader = GetShaderModule(desc.vertexShader);
    const auto* fragmentShader = GetShaderModule(desc.fragmentShader);
    if (!vertexShader || !fragmentShader)
        return {};

    PipelineResource pipelineResource;
    pipelineResource.compute = false;
    pipelineResource.topology = desc.topology;
    for (const auto& binding : desc.vertexInput.bindings)
        pipelineResource.vertexStrides[binding.binding] = binding.stride;

    if (!CreateRootSignature(desc.descriptorSetLayouts,
                             ShaderStage::Vertex | ShaderStage::Fragment | ShaderStage::Geometry, desc.pushConstantSize,
                             pipelineResource.rootSignature, pipelineResource.rootBindings,
                             pipelineResource.pushConstantsRootIndex))
        return {};

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    std::vector<std::string> semanticNames;
    semanticNames.reserve(desc.vertexInput.attributes.size());
    inputElements.reserve(desc.vertexInput.attributes.size());
    for (const auto& attribute : desc.vertexInput.attributes)
    {
        semanticNames.push_back("ATTRIBUTE" + std::to_string(attribute.location));
        D3D12_INPUT_ELEMENT_DESC element{};
        element.SemanticName = semanticNames.back().c_str();
        element.SemanticIndex = 0;
        element.Format = ToD3DVertexFormat(attribute.format);
        element.InputSlot = attribute.binding;
        element.AlignedByteOffset = attribute.offset;
        element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element.InstanceDataStepRate = 0;
        auto bindingIt =
            std::find_if(desc.vertexInput.bindings.begin(), desc.vertexInput.bindings.end(),
                         [&](const VertexBindingDesc& binding) { return binding.binding == attribute.binding; });
        if (bindingIt != desc.vertexInput.bindings.end() && bindingIt->inputRate == VertexInputRate::PerInstance)
        {
            element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            element.InstanceDataStepRate = 1;
        }
        inputElements.push_back(element);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = pipelineResource.rootSignature.Get();
    pso.VS = {vertexShader->bytecode.data(), vertexShader->bytecode.size()};
    pso.PS = {fragmentShader->bytecode.data(), fragmentShader->bytecode.size()};
    if (const auto* geometryShader = GetShaderModule(desc.geometryShader))
        pso.GS = {geometryShader->bytecode.data(), geometryShader->bytecode.size()};
    pso.InputLayout = {inputElements.data(), static_cast<UINT>(inputElements.size())};
    pso.PrimitiveTopologyType = ToD3DTopologyType(desc.topology);
    pso.RasterizerState.FillMode = ToD3DFillMode(desc.rasterizer.polygonMode);
    pso.RasterizerState.CullMode = ToD3DCullMode(desc.rasterizer.cullMode);
    pso.RasterizerState.FrontCounterClockwise = desc.rasterizer.frontFace == FrontFace::CounterClockwise;
    pso.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    pso.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    pso.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.RasterizerState.MultisampleEnable = FALSE;
    pso.RasterizerState.AntialiasedLineEnable = FALSE;
    pso.RasterizerState.ForcedSampleCount = 0;
    pso.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    pso.BlendState.AlphaToCoverageEnable = FALSE;
    pso.BlendState.IndependentBlendEnable = desc.blendAttachments.size() > 1;
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        BlendAttachmentDesc blend = i < desc.blendAttachments.size() ? desc.blendAttachments[i] : BlendAttachmentDesc{};
        auto& target = pso.BlendState.RenderTarget[i];
        target.BlendEnable = blend.blendEnable;
        target.LogicOpEnable = FALSE;
        target.SrcBlend = ToD3DBlend(blend.srcColorBlendFactor);
        target.DestBlend = ToD3DBlend(blend.dstColorBlendFactor);
        target.BlendOp = ToD3DBlendOp(blend.colorBlendOp);
        target.SrcBlendAlpha = ToD3DBlend(blend.srcAlphaBlendFactor);
        target.DestBlendAlpha = ToD3DBlend(blend.dstAlphaBlendFactor);
        target.BlendOpAlpha = ToD3DBlendOp(blend.alphaBlendOp);
        target.LogicOp = D3D12_LOGIC_OP_NOOP;
        target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    pso.DepthStencilState.DepthEnable = desc.depthStencil.depthTestEnable;
    pso.DepthStencilState.DepthWriteMask =
        desc.depthStencil.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    pso.DepthStencilState.DepthFunc = ToD3DCompareOp(desc.depthStencil.depthCompare);
    pso.DepthStencilState.StencilEnable = desc.depthStencil.stencilTestEnable;
    pso.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    pso.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    pso.SampleMask = UINT_MAX;
    pso.NumRenderTargets = static_cast<UINT>(desc.renderTargetLayout.colorFormats.size());
    for (UINT i = 0; i < pso.NumRenderTargets; ++i)
        pso.RTVFormats[i] = ToDxgiFormat(desc.renderTargetLayout.colorFormats[i]);
    pso.DSVFormat = desc.renderTargetLayout.depthStencilFormat != Format::Undefined
                        ? ToDxgiFormat(desc.renderTargetLayout.depthStencilFormat)
                        : DXGI_FORMAT_UNKNOWN;
    pso.SampleDesc.Count = 1;

    if (!Check(m_Device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineResource.pipeline)),
               "CreateGraphicsPipelineState"))
        return {};

    const uint32_t id = AllocateHandle();
    m_Pipelines[id] = std::move(pipelineResource);
    return PipelineHandle{id};
}

PipelineHandle DirectX12RenderDevice::CreateComputePipeline(const ComputePipelineDesc& desc)
{
    const auto* computeShader = GetShaderModule(desc.computeShader);
    if (!computeShader)
        return {};

    PipelineResource pipelineResource;
    pipelineResource.compute = true;
    if (!CreateRootSignature(desc.descriptorSetLayouts, ShaderStage::Compute, desc.pushConstantSize,
                             pipelineResource.rootSignature, pipelineResource.rootBindings,
                             pipelineResource.pushConstantsRootIndex))
        return {};

    D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature = pipelineResource.rootSignature.Get();
    pso.CS = {computeShader->bytecode.data(), computeShader->bytecode.size()};
    if (!Check(m_Device->CreateComputePipelineState(&pso, IID_PPV_ARGS(&pipelineResource.pipeline)),
               "CreateComputePipelineState"))
        return {};

    const uint32_t id = AllocateHandle();
    m_Pipelines[id] = std::move(pipelineResource);
    return PipelineHandle{id};
}

void DirectX12RenderDevice::DestroyPipeline(PipelineHandle handle)
{
    m_Pipelines.erase(handle.id);
}

ICommandList& DirectX12RenderDevice::BeginCommandList(CommandQueueType queue)
{
    (void)queue;
    m_CommandList.Begin();
    return m_CommandList;
}

void DirectX12RenderDevice::Submit(ICommandList& commandList)
{
    commandList.End();
    auto* d3dCommandList = dynamic_cast<DirectX12CommandList*>(&commandList);
    if (!d3dCommandList)
        return;
    ID3D12CommandList* lists[] = {d3dCommandList->GetNative()};
    m_GraphicsQueue->ExecuteCommandLists(1, lists);
    WaitIdle();
}

void DirectX12RenderDevice::WaitIdle()
{
    if (!m_GraphicsQueue || !m_Fence)
        return;

    const uint64_t fenceValue = ++m_FenceValue;
    if (!Check(m_GraphicsQueue->Signal(m_Fence.Get(), fenceValue), "ID3D12CommandQueue::Signal"))
        return;
    if (m_Fence->GetCompletedValue() >= fenceValue)
        return;

    HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!eventHandle)
        return;
    if (Check(m_Fence->SetEventOnCompletion(fenceValue, eventHandle), "ID3D12Fence::SetEventOnCompletion"))
        WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
}

BackendType DirectX12RenderDevice::GetBackendType() const
{
    return BackendType::DirectX;
}

const char* DirectX12RenderDevice::GetName() const
{
    return "DirectX 12";
}

uint32_t DirectX12RenderDevice::AllocateHandle()
{
    return m_NextHandle++;
}

uint32_t DirectX12RenderDevice::AllocateCbvSrvUav(uint32_t count)
{
    const uint32_t index = m_NextCbvSrvUav;
    m_NextCbvSrvUav += count;
    return index;
}

uint32_t DirectX12RenderDevice::AllocateSampler(uint32_t count)
{
    const uint32_t index = m_NextSampler;
    m_NextSampler += count;
    return index;
}

uint32_t DirectX12RenderDevice::AllocateRtv(uint32_t count)
{
    const uint32_t index = m_NextRtv;
    m_NextRtv += count;
    return index;
}

uint32_t DirectX12RenderDevice::AllocateDsv(uint32_t count)
{
    const uint32_t index = m_NextDsv;
    m_NextDsv += count;
    return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12RenderDevice::GetCbvSrvUavCpu(uint32_t index) const
{
    auto handle = m_CbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * m_CbvSrvUavDescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12RenderDevice::GetCbvSrvUavGpu(uint32_t index) const
{
    if (index == UINT32_MAX)
        return {};
    auto handle = m_CbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * m_CbvSrvUavDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12RenderDevice::GetSamplerCpu(uint32_t index) const
{
    auto handle = m_SamplerHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * m_SamplerDescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12RenderDevice::GetSamplerGpu(uint32_t index) const
{
    if (index == UINT32_MAX)
        return {};
    auto handle = m_SamplerHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(index) * m_SamplerDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12RenderDevice::GetRtvCpu(uint32_t index) const
{
    auto handle = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * m_RtvDescriptorSize;
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12RenderDevice::GetDsvCpu(uint32_t index) const
{
    auto handle = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(index) * m_DsvDescriptorSize;
    return handle;
}

void DirectX12RenderDevice::ExecuteImmediate(ID3D12CommandList* commandList)
{
    m_GraphicsQueue->ExecuteCommandLists(1, &commandList);
    WaitIdle();
}

void DirectX12RenderDevice::TransitionResource(ID3D12GraphicsCommandList* commandList, ImageResource& image,
                                               D3D12_RESOURCE_STATES newState)
{
    if (!commandList || image.state == newState)
        return;

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = image.resource.Get();
    barrier.Transition.StateBefore = image.state;
    barrier.Transition.StateAfter = newState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);
    image.state = newState;
}

bool DirectX12RenderDevice::UploadTextureData(ImageResource& image, const void* initialData)
{
    if (!initialData)
        return true;

    const auto resourceDesc = image.resource->GetDesc();
    const uint32_t subresourceCount =
        static_cast<uint32_t>(resourceDesc.MipLevels) *
        (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : resourceDesc.DepthOrArraySize);
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(subresourceCount);
    std::vector<UINT> rowCounts(subresourceCount);
    std::vector<UINT64> rowSizes(subresourceCount);
    UINT64 uploadSize = 0;
    m_Device->GetCopyableFootprints(&resourceDesc, 0, subresourceCount, 0, layouts.data(), rowCounts.data(),
                                    rowSizes.data(), &uploadSize);

    BufferDesc uploadDesc;
    uploadDesc.size = uploadSize;
    uploadDesc.usage = BufferUsage::TransferSrc;
    uploadDesc.memoryUsage = MemoryUsage::CpuToGpu;
    BufferHandle uploadHandle = CreateBuffer(uploadDesc);
    auto* upload = GetBuffer(uploadHandle);
    if (!upload)
        return false;

    void* mapped = nullptr;
    D3D12_RANGE readRange{0, 0};
    if (!Check(upload->resource->Map(0, &readRange, &mapped), "Map(TextureUpload)"))
    {
        DestroyBuffer(uploadHandle);
        return false;
    }

    const uint8_t* src = static_cast<const uint8_t*>(initialData);
    const uint32_t bytesPerPixel = GetFormatByteSize(image.desc.format);
    for (uint32_t subresource = 0; subresource < subresourceCount; ++subresource)
    {
        const auto& footprint = layouts[subresource].Footprint;
        uint8_t* dst = static_cast<uint8_t*>(mapped) + layouts[subresource].Offset;
        const uint32_t mip = subresource % image.desc.mipLevels;
        const uint32_t width = std::max(1u, image.desc.width >> mip);
        const uint32_t height = std::max(1u, image.desc.height >> mip);
        const uint32_t depth =
            resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? std::max(1u, image.desc.depth >> mip) : 1u;
        const uint64_t tightRowSize = static_cast<uint64_t>(width) * bytesPerPixel;
        for (uint32_t z = 0; z < depth; ++z)
        {
            for (uint32_t y = 0; y < height; ++y)
            {
                std::memcpy(dst + z * footprint.RowPitch * height + y * footprint.RowPitch, src,
                            static_cast<size_t>(tightRowSize));
                src += tightRowSize;
            }
        }
    }
    D3D12_RANGE writeRange{0, static_cast<SIZE_T>(uploadSize)};
    upload->resource->Unmap(0, &writeRange);

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    if (!Check(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
               "CreateCommandAllocator(TextureUpload)") ||
        !Check(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                           IID_PPV_ARGS(&commandList)),
               "CreateCommandList(TextureUpload)"))
    {
        DestroyBuffer(uploadHandle);
        return false;
    }

    for (uint32_t subresource = 0; subresource < subresourceCount; ++subresource)
    {
        D3D12_TEXTURE_COPY_LOCATION dst{};
        dst.pResource = image.resource.Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = subresource;

        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        srcLocation.pResource = upload->resource.Get();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint = layouts[subresource];
        commandList->CopyTextureRegion(&dst, 0, 0, 0, &srcLocation, nullptr);
    }

    D3D12_RESOURCE_STATES finalState = D3D12_RESOURCE_STATE_COMMON;
    if (HasFlag(image.desc.usage, ImageUsage::Sampled))
        finalState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    else if (HasFlag(image.desc.usage, ImageUsage::ColorAttachment))
        finalState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    else if (HasFlag(image.desc.usage, ImageUsage::DepthStencilAttachment))
        finalState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    else if (HasFlag(image.desc.usage, ImageUsage::Storage))
        finalState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    image.state = D3D12_RESOURCE_STATE_COPY_DEST;
    TransitionResource(commandList.Get(), image, finalState);

    commandList->Close();
    ID3D12CommandList* lists[] = {commandList.Get()};
    ExecuteImmediate(lists[0]);
    DestroyBuffer(uploadHandle);
    return true;
}

bool DirectX12RenderDevice::CreateRootSignature(const std::vector<DescriptorSetLayoutHandle>& layouts,
                                                ShaderStage pushConstantStages, uint32_t pushConstantSize,
                                                Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature,
                                                std::vector<RootBinding>& rootBindings,
                                                uint32_t& pushConstantsRootIndex)
{
    uint32_t rangeCount = 0;
    uint32_t rootParameterCount = 0;
    for (const auto& layoutHandle : layouts)
    {
        const auto* layout = GetDescriptorSetLayout(layoutHandle);
        if (!layout)
            continue;
        for (const auto& binding : layout->desc.bindings)
        {
            rangeCount += binding.type == DescriptorType::CombinedImageSampler ? 2u : 1u;
            rootParameterCount += binding.type == DescriptorType::CombinedImageSampler ? 2u : 1u;
        }
    }
    if (pushConstantSize > 0)
        ++rootParameterCount;

    std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
    std::vector<D3D12_ROOT_PARAMETER> rootParameters;
    ranges.reserve(rangeCount);
    rootParameters.reserve(rootParameterCount);
    rootBindings.clear();
    pushConstantsRootIndex = UINT32_MAX;

    for (uint32_t setIndex = 0; setIndex < layouts.size(); ++setIndex)
    {
        const auto* layout = GetDescriptorSetLayout(layouts[setIndex]);
        if (!layout)
            continue;

        for (const auto& binding : layout->desc.bindings)
        {
            auto addTable = [&](bool samplerPart) {
                D3D12_DESCRIPTOR_RANGE range{};
                range.RangeType = ToDescriptorRangeType(binding.type, samplerPart);
                range.NumDescriptors = std::max(binding.count, 1u);
                range.BaseShaderRegister = binding.binding;
                range.RegisterSpace = setIndex;
                range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                ranges.push_back(range);

                D3D12_ROOT_PARAMETER parameter{};
                parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                parameter.DescriptorTable.NumDescriptorRanges = 1;
                parameter.DescriptorTable.pDescriptorRanges = &ranges.back();
                parameter.ShaderVisibility = ToShaderVisibility(binding.stages);
                const uint32_t rootIndex = static_cast<uint32_t>(rootParameters.size());
                rootParameters.push_back(parameter);
                rootBindings.push_back(RootBinding{setIndex, binding.binding, rootIndex, samplerPart});
            };

            if (binding.type == DescriptorType::CombinedImageSampler)
            {
                addTable(false);
                addTable(true);
            }
            else
            {
                addTable(binding.type == DescriptorType::Sampler);
            }
        }
    }

    if (pushConstantSize > 0)
    {
        D3D12_ROOT_PARAMETER constants{};
        constants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        constants.Constants.ShaderRegister = 255;
        constants.Constants.RegisterSpace = 0;
        constants.Constants.Num32BitValues =
            static_cast<UINT>((pushConstantSize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
        constants.ShaderVisibility = ToShaderVisibility(pushConstantStages);
        pushConstantsRootIndex = static_cast<uint32_t>(rootParameters.size());
        rootParameters.push_back(constants);
    }

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = static_cast<UINT>(rootParameters.size());
    desc.pParameters = rootParameters.data();
    desc.NumStaticSamplers = 0;
    desc.pStaticSamplers = nullptr;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    if (!Check(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error),
               "D3D12SerializeRootSignature"))
    {
        if (error)
            LOGGER_ERROR("DirectX12RHI") << static_cast<const char*>(error->GetBufferPointer());
        return false;
    }

    return Check(m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                               IID_PPV_ARGS(&rootSignature)),
                 "CreateRootSignature");
}

ImageHandle DirectX12RenderDevice::RegisterSwapchainImage(ID3D12Resource* resource, const ImageDesc& desc)
{
    const uint32_t id = AllocateHandle();
    ImageResource image;
    image.resource = resource;
    image.desc = desc;
    image.state = D3D12_RESOURCE_STATE_PRESENT;
    image.owned = false;
    const uint32_t rtvIndex = AllocateRtv();
    image.rtv = GetRtvCpu(rtvIndex);
    m_Device->CreateRenderTargetView(resource, nullptr, image.rtv);
    m_Images[id] = image;
    return ImageHandle{id};
}

void DirectX12RenderDevice::UnregisterImage(ImageHandle handle)
{
    m_Images.erase(handle.id);
}

const DirectX12RenderDevice::BufferResource* DirectX12RenderDevice::GetBuffer(BufferHandle handle) const
{
    auto it = m_Buffers.find(handle.id);
    return it == m_Buffers.end() ? nullptr : &it->second;
}

DirectX12RenderDevice::BufferResource* DirectX12RenderDevice::GetBuffer(BufferHandle handle)
{
    auto it = m_Buffers.find(handle.id);
    return it == m_Buffers.end() ? nullptr : &it->second;
}

const DirectX12RenderDevice::ImageResource* DirectX12RenderDevice::GetImage(ImageHandle handle) const
{
    auto it = m_Images.find(handle.id);
    return it == m_Images.end() ? nullptr : &it->second;
}

DirectX12RenderDevice::ImageResource* DirectX12RenderDevice::GetImage(ImageHandle handle)
{
    auto it = m_Images.find(handle.id);
    return it == m_Images.end() ? nullptr : &it->second;
}

const DirectX12RenderDevice::SamplerResource* DirectX12RenderDevice::GetSampler(SamplerHandle handle) const
{
    auto it = m_Samplers.find(handle.id);
    return it == m_Samplers.end() ? nullptr : &it->second;
}

const DirectX12RenderDevice::ShaderModuleResource* DirectX12RenderDevice::GetShaderModule(
    ShaderModuleHandle handle) const
{
    auto it = m_ShaderModules.find(handle.id);
    return it == m_ShaderModules.end() ? nullptr : &it->second;
}

const DirectX12RenderDevice::DescriptorSetLayoutResource* DirectX12RenderDevice::GetDescriptorSetLayout(
    DescriptorSetLayoutHandle handle) const
{
    auto it = m_DescriptorSetLayouts.find(handle.id);
    return it == m_DescriptorSetLayouts.end() ? nullptr : &it->second;
}

const DirectX12RenderDevice::DescriptorSetResource* DirectX12RenderDevice::GetDescriptorSet(
    DescriptorSetHandle handle) const
{
    auto it = m_DescriptorSets.find(handle.id);
    return it == m_DescriptorSets.end() ? nullptr : &it->second;
}

DirectX12RenderDevice::DescriptorSetResource* DirectX12RenderDevice::GetDescriptorSet(DescriptorSetHandle handle)
{
    auto it = m_DescriptorSets.find(handle.id);
    return it == m_DescriptorSets.end() ? nullptr : &it->second;
}

const DirectX12RenderDevice::PipelineResource* DirectX12RenderDevice::GetPipeline(PipelineHandle handle) const
{
    auto it = m_Pipelines.find(handle.id);
    return it == m_Pipelines.end() ? nullptr : &it->second;
}

}  // namespace rhi
#endif