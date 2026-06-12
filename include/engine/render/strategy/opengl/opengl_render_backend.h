#pragma once

#include <render/rhi/i_render_backend.h>
#include <unordered_map>

class OpenGLRenderDevice;

class OpenGLCommandList final : public rhi::ICommandList
{
public:
    explicit OpenGLCommandList(OpenGLRenderDevice& device);
    ~OpenGLCommandList() override;

    void Begin() override;
    void End() override;

    void BeginRendering(const rhi::RenderPassBeginInfo& beginInfo) override;
    void EndRendering() override;

    void SetViewport(const rhi::Viewport& viewport) override;
    void SetScissor(const rhi::Rect2D& scissor) override;

    void BindPipeline(rhi::PipelineHandle pipeline) override;
    void BindDescriptorSet(uint32_t setIndex, rhi::DescriptorSetHandle descriptorSet) override;
    void BindVertexBuffer(uint32_t binding, rhi::BufferHandle buffer, uint64_t offset = 0) override;
    void BindIndexBuffer(rhi::BufferHandle buffer, rhi::IndexType indexType, uint64_t offset = 0) override;

    void PushConstants(rhi::ShaderStage stages, const void* data, uint32_t size, uint32_t offset = 0) override;

    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
              uint32_t firstInstance = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                     uint32_t firstInstance = 0) override;
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

private:
    struct BoundVertexBuffer
    {
        rhi::BufferHandle buffer;
        uint64_t offset = 0;
    };

    void ApplyPipelineState();
    void ApplyVertexInput();
    void BindDescriptorUpdate(const rhi::DescriptorUpdate& update);

    OpenGLRenderDevice& m_Device;
    rhi::PipelineHandle m_CurrentPipeline;
    rhi::IndexType m_IndexType = rhi::IndexType::UInt32;
    uint64_t m_IndexOffset = 0;
    uint32_t m_RenderFramebuffer = 0;
    bool m_IsRecording = false;
    std::unordered_map<uint32_t, BoundVertexBuffer> m_VertexBuffers;
};

class OpenGLSwapchain final : public rhi::ISwapchain
{
public:
    void Resize(uint32_t width, uint32_t height) override;
    rhi::Extent2D GetExtent() const override;
    rhi::Format GetBackBufferFormat() const override;
    rhi::ImageHandle GetCurrentBackBuffer() const override;

private:
    rhi::Extent2D m_Extent;
    rhi::Format m_BackBufferFormat = rhi::Format::RGBA8;
};

class OpenGLRenderDevice final : public rhi::IRenderDevice
{
public:
    OpenGLRenderDevice();
    ~OpenGLRenderDevice() override;

    void Initialize(uint32_t defaultVertexArray);
    void Shutdown();

    rhi::BufferHandle CreateBuffer(const rhi::BufferDesc& desc, const void* initialData = nullptr) override;
    void UpdateBuffer(rhi::BufferHandle handle, uint64_t offset, uint64_t size, const void* data) override;
    void DestroyBuffer(rhi::BufferHandle handle) override;

    rhi::ImageHandle CreateImage(const rhi::ImageDesc& desc, const void* initialData = nullptr) override;
    void DestroyImage(rhi::ImageHandle handle) override;

    rhi::SamplerHandle CreateSampler(const rhi::SamplerDesc& desc) override;
    void DestroySampler(rhi::SamplerHandle handle) override;

    rhi::ShaderModuleHandle CreateShaderModule(const rhi::ShaderModuleDesc& desc) override;
    void DestroyShaderModule(rhi::ShaderModuleHandle handle) override;

    rhi::DescriptorSetLayoutHandle CreateDescriptorSetLayout(const rhi::DescriptorSetLayoutDesc& desc) override;
    void DestroyDescriptorSetLayout(rhi::DescriptorSetLayoutHandle handle) override;

    rhi::DescriptorSetHandle CreateDescriptorSet(rhi::DescriptorSetLayoutHandle layout) override;
    void UpdateDescriptorSet(rhi::DescriptorSetHandle set, const rhi::DescriptorUpdate* updates,
                             uint32_t count) override;
    void DestroyDescriptorSet(rhi::DescriptorSetHandle handle) override;

    rhi::PipelineHandle CreateGraphicsPipeline(const rhi::GraphicsPipelineDesc& desc) override;
    rhi::PipelineHandle CreateComputePipeline(const rhi::ComputePipelineDesc& desc) override;
    void DestroyPipeline(rhi::PipelineHandle handle) override;

    rhi::ICommandList& BeginCommandList(rhi::CommandQueueType queue = rhi::CommandQueueType::Graphics) override;
    void Submit(rhi::ICommandList& commandList) override;
    void WaitIdle() override;

    rhi::BackendType GetBackendType() const override;
    const char* GetName() const override;

private:
    friend class OpenGLCommandList;

    struct BufferResource
    {
        uint32_t object = 0;
        uint32_t target = 0;
        uint64_t size = 0;
        rhi::BufferUsage usage = rhi::BufferUsage::None;
    };

    struct ImageResource
    {
        uint32_t object = 0;
        uint32_t target = 0;
        rhi::ImageDesc desc;
    };

    struct SamplerResource
    {
        uint32_t object = 0;
    };

    struct ShaderModuleResource
    {
        uint32_t object = 0;
        rhi::ShaderStage stage = rhi::ShaderStage::None;
    };

    struct DescriptorSetLayoutResource
    {
        rhi::DescriptorSetLayoutDesc desc;
    };

    struct DescriptorSetResource
    {
        rhi::DescriptorSetLayoutHandle layout;
        std::vector<rhi::DescriptorUpdate> updates;
    };

    struct PipelineResource
    {
        uint32_t program = 0;
        bool compute = false;
        rhi::GraphicsPipelineDesc graphicsDesc;
        rhi::ComputePipelineDesc computeDesc;
    };

    uint32_t AllocateHandle();
    const BufferResource* GetBuffer(rhi::BufferHandle handle) const;
    const ImageResource* GetImage(rhi::ImageHandle handle) const;
    const SamplerResource* GetSampler(rhi::SamplerHandle handle) const;
    const ShaderModuleResource* GetShaderModule(rhi::ShaderModuleHandle handle) const;
    const DescriptorSetResource* GetDescriptorSet(rhi::DescriptorSetHandle handle) const;
    const PipelineResource* GetPipeline(rhi::PipelineHandle handle) const;

    std::unordered_map<uint32_t, BufferResource> m_Buffers;
    std::unordered_map<uint32_t, ImageResource> m_Images;
    std::unordered_map<uint32_t, SamplerResource> m_Samplers;
    std::unordered_map<uint32_t, ShaderModuleResource> m_ShaderModules;
    std::unordered_map<uint32_t, DescriptorSetLayoutResource> m_DescriptorSetLayouts;
    std::unordered_map<uint32_t, DescriptorSetResource> m_DescriptorSets;
    std::unordered_map<uint32_t, PipelineResource> m_Pipelines;

    uint32_t m_NextHandle = 1;
    uint32_t m_DefaultVertexArray = 0;
    OpenGLCommandList m_CommandList;
};

class OpenGLRenderBackend final : public rhi::IRenderBackend
{
public:
    OpenGLRenderBackend();
    ~OpenGLRenderBackend() override;

    bool Initialize(const rhi::RenderBackendCreateInfo& createInfo) override;
    void Shutdown() override;

    bool BeginFrame() override;
    void EndFrame() override;
    void OnResize(uint32_t width, uint32_t height) override;

    rhi::IRenderDevice& GetDevice() override;
    rhi::ISwapchain& GetSwapchain() override;

    rhi::BackendType GetBackendType() const override;
    const char* GetName() const override;

private:
    OpenGLRenderDevice m_Device;
    OpenGLSwapchain m_Swapchain;
    uint32_t m_DefaultVertexArray = 0;
    bool m_Initialized = false;
};
