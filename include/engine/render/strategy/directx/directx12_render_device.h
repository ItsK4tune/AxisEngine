#pragma once
#include <render/rhi/i_render_device.h>
#include <render/strategy/directx/directx12_command_list.h>
#include <render/strategy/directx/directx12_common.h>
#if AXIS_HAS_DIRECTX_BACKEND
#include <array>
#include <unordered_map>
#include <vector>

namespace rhi
{
class DirectX12RenderDevice final : public IRenderDevice
{
public:
    DirectX12RenderDevice();
    ~DirectX12RenderDevice() override;

    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* graphicsQueue);
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
    friend class DirectX12CommandList;
    friend class DirectX12RenderBackend;

    struct BufferResource
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint64_t size = 0;
        BufferUsage usage = BufferUsage::None;
        MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    };

    struct ImageResource
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        ImageDesc desc;
        D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
        D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
        D3D12_CPU_DESCRIPTOR_HANDLE srv{};
        D3D12_CPU_DESCRIPTOR_HANDLE uav{};
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
        bool owned = true;
    };

    struct SamplerResource
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
    };

    struct ShaderModuleResource
    {
        std::vector<uint8_t> bytecode;
        ShaderStage stage = ShaderStage::None;
    };

    struct DescriptorSetLayoutResource
    {
        DescriptorSetLayoutDesc desc;
    };

    struct DescriptorSlot
    {
        uint32_t cbvSrvUavIndex = UINT32_MAX;
        uint32_t samplerIndex = UINT32_MAX;
        uint32_t count = 1;
        DescriptorType type = DescriptorType::UniformBuffer;
    };

    struct DescriptorSetResource
    {
        DescriptorSetLayoutHandle layout;
        std::unordered_map<uint32_t, DescriptorSlot> slots;
        std::vector<DescriptorUpdate> updates;
    };

    struct RootBinding
    {
        uint32_t set = 0;
        uint32_t binding = 0;
        uint32_t rootIndex = 0;
        bool sampler = false;
    };

    struct PipelineResource
    {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
        bool compute = false;
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        uint32_t pushConstantsRootIndex = UINT32_MAX;
        std::vector<RootBinding> rootBindings;
        std::unordered_map<uint32_t, uint32_t> vertexStrides;
    };

    uint32_t AllocateHandle();
    uint32_t AllocateCbvSrvUav(uint32_t count = 1);
    uint32_t AllocateSampler(uint32_t count = 1);
    uint32_t AllocateRtv(uint32_t count = 1);
    uint32_t AllocateDsv(uint32_t count = 1);
    D3D12_CPU_DESCRIPTOR_HANDLE GetCbvSrvUavCpu(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetCbvSrvUavGpu(uint32_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerCpu(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerGpu(uint32_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCpu(uint32_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCpu(uint32_t index) const;
    void ExecuteImmediate(ID3D12CommandList* commandList);
    void TransitionResource(ID3D12GraphicsCommandList* commandList, ImageResource& image,
                            D3D12_RESOURCE_STATES newState);
    bool UploadTextureData(ImageResource& image, const void* initialData);
    bool CreateRootSignature(const std::vector<DescriptorSetLayoutHandle>& layouts, ShaderStage pushConstantStages,
                             uint32_t pushConstantSize, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSignature,
                             std::vector<RootBinding>& rootBindings, uint32_t& pushConstantsRootIndex);
    ImageHandle RegisterSwapchainImage(ID3D12Resource* resource, const ImageDesc& desc);
    void UnregisterImage(ImageHandle handle);

    const BufferResource* GetBuffer(BufferHandle handle) const;
    BufferResource* GetBuffer(BufferHandle handle);
    const ImageResource* GetImage(ImageHandle handle) const;
    ImageResource* GetImage(ImageHandle handle);
    const SamplerResource* GetSampler(SamplerHandle handle) const;
    const ShaderModuleResource* GetShaderModule(ShaderModuleHandle handle) const;
    const DescriptorSetLayoutResource* GetDescriptorSetLayout(DescriptorSetLayoutHandle handle) const;
    const DescriptorSetResource* GetDescriptorSet(DescriptorSetHandle handle) const;
    DescriptorSetResource* GetDescriptorSet(DescriptorSetHandle handle);
    const PipelineResource* GetPipeline(PipelineHandle handle) const;

    Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_GraphicsQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    uint64_t m_FenceValue = 0;
    DirectX12CommandList m_CommandList;
    uint32_t m_NextHandle = 1;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CbvSrvUavHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SamplerHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DsvHeap;
    uint32_t m_CbvSrvUavDescriptorSize = 0;
    uint32_t m_SamplerDescriptorSize = 0;
    uint32_t m_RtvDescriptorSize = 0;
    uint32_t m_DsvDescriptorSize = 0;
    uint32_t m_NextCbvSrvUav = 0;
    uint32_t m_NextSampler = 0;
    uint32_t m_NextRtv = 0;
    uint32_t m_NextDsv = 0;

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