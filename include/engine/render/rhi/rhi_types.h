#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rhi
{
enum class BackendType
{
    OpenGL,
    Vulkan,
    DirectX
};

enum class CommandQueueType
{
    Graphics,
    Compute,
    Transfer
};

enum class MemoryUsage
{
    GpuOnly,
    CpuToGpu,
    GpuToCpu
};

enum class BufferUsage : uint32_t
{
    None = 0,
    Vertex = 1 << 0,
    Index = 1 << 1,
    Uniform = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
    Indirect = 1 << 6
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b)
{
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasFlag(BufferUsage flags, BufferUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

enum class ImageUsage : uint32_t
{
    None = 0,
    Sampled = 1 << 0,
    ColorAttachment = 1 << 1,
    DepthStencilAttachment = 1 << 2,
    Storage = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
    Present = 1 << 6
};

inline ImageUsage operator|(ImageUsage a, ImageUsage b)
{
    return static_cast<ImageUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasFlag(ImageUsage flags, ImageUsage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

enum class Format
{
    Undefined,
    R8,
    RG8,
    RGB8,
    RGBA8,
    BGRA8,
    RGBA16F,
    R32UInt,
    D24S8,
    Depth24,
    Depth32F
};

enum class ImageDimension
{
    Image1D,
    Image2D,
    Image3D,
    Cube
};

enum class Filter
{
    Nearest,
    Linear
};

enum class MipmapMode
{
    Nearest,
    Linear
};

enum class AddressMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class ShaderStage : uint32_t
{
    None = 0,
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Geometry = 1 << 2,
    Compute = 1 << 3
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b)
{
    return static_cast<ShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool HasFlag(ShaderStage flags, ShaderStage flag)
{
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

enum class ShaderSourceType
{
    SourceText,
    SpirVBinary,
    DxilBinary
};

enum class PrimitiveTopology
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip
};

enum class IndexType
{
    UInt16,
    UInt32
};

enum class VertexFormat
{
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4
};

enum class VertexInputRate
{
    PerVertex,
    PerInstance
};

enum class CompareOp
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class CullMode
{
    None,
    Front,
    Back
};

enum class FrontFace
{
    Clockwise,
    CounterClockwise
};

enum class PolygonMode
{
    Fill,
    Line,
    Point
};

enum class BlendFactor
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstColor,
    OneMinusDstColor,
    DstAlpha,
    OneMinusDstAlpha
};

enum class BlendOp
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class LoadOp
{
    Load,
    Clear,
    DontCare
};

enum class StoreOp
{
    Store,
    DontCare
};

enum class DescriptorType
{
    UniformBuffer,
    StorageBuffer,
    SampledImage,
    StorageImage,
    Sampler,
    CombinedImageSampler
};

template <typename Tag>
struct Handle
{
    uint32_t id = 0;

    bool IsValid() const
    {
        return id != 0;
    }

    explicit operator bool() const
    {
        return IsValid();
    }

    bool operator==(const Handle& other) const
    {
        return id == other.id;
    }

    bool operator!=(const Handle& other) const
    {
        return id != other.id;
    }
};

struct BufferTag;
struct ImageTag;
struct SamplerTag;
struct ShaderModuleTag;
struct PipelineTag;
struct DescriptorSetLayoutTag;
struct DescriptorSetTag;

using BufferHandle = Handle<BufferTag>;
using ImageHandle = Handle<ImageTag>;
using SamplerHandle = Handle<SamplerTag>;
using ShaderModuleHandle = Handle<ShaderModuleTag>;
using PipelineHandle = Handle<PipelineTag>;
using DescriptorSetLayoutHandle = Handle<DescriptorSetLayoutTag>;
using DescriptorSetHandle = Handle<DescriptorSetTag>;

struct Extent2D
{
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Rect2D
{
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Viewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct ClearColor
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct ClearDepthStencil
{
    float depth = 1.0f;
    uint32_t stencil = 0;
};

struct BufferDesc
{
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::None;
    MemoryUsage memoryUsage = MemoryUsage::GpuOnly;
    std::string debugName;
};

struct ImageDesc
{
    ImageDimension dimension = ImageDimension::Image2D;
    Format format = Format::RGBA8;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    ImageUsage usage = ImageUsage::Sampled;
    std::string debugName;
};

struct SamplerDesc
{
    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;
    MipmapMode mipmapMode = MipmapMode::Linear;
    AddressMode addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    AddressMode addressW = AddressMode::Repeat;
    float maxAnisotropy = 1.0f;
    std::string debugName;
};

struct ShaderModuleDesc
{
    ShaderStage stage = ShaderStage::None;
    ShaderSourceType sourceType = ShaderSourceType::SourceText;
    const void* data = nullptr;
    uint64_t size = 0;
    std::string entryPoint = "main";
    std::string debugName;
};

struct VertexBindingDesc
{
    uint32_t binding = 0;
    uint32_t stride = 0;
    VertexInputRate inputRate = VertexInputRate::PerVertex;
};

struct VertexAttributeDesc
{
    uint32_t location = 0;
    uint32_t binding = 0;
    VertexFormat format = VertexFormat::Float3;
    uint32_t offset = 0;
};

struct VertexInputStateDesc
{
    std::vector<VertexBindingDesc> bindings;
    std::vector<VertexAttributeDesc> attributes;
};

struct RasterizerStateDesc
{
    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::CounterClockwise;
    PolygonMode polygonMode = PolygonMode::Fill;
    float lineWidth = 1.0f;
};

struct DepthStencilStateDesc
{
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    CompareOp depthCompare = CompareOp::Less;
    bool stencilTestEnable = false;
};

struct BlendAttachmentDesc
{
    bool blendEnable = false;
    BlendFactor srcColorBlendFactor = BlendFactor::One;
    BlendFactor dstColorBlendFactor = BlendFactor::Zero;
    BlendOp colorBlendOp = BlendOp::Add;
    BlendFactor srcAlphaBlendFactor = BlendFactor::One;
    BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
    BlendOp alphaBlendOp = BlendOp::Add;
};

struct RenderTargetLayoutDesc
{
    std::vector<Format> colorFormats;
    Format depthStencilFormat = Format::Undefined;
};

struct DescriptorBindingDesc
{
    uint32_t binding = 0;
    DescriptorType type = DescriptorType::UniformBuffer;
    ShaderStage stages = ShaderStage::Vertex;
    uint32_t count = 1;
};

struct DescriptorSetLayoutDesc
{
    std::vector<DescriptorBindingDesc> bindings;
    std::string debugName;
};

struct DescriptorUpdate
{
    uint32_t binding = 0;
    uint32_t arrayElement = 0;
    DescriptorType type = DescriptorType::UniformBuffer;
    BufferHandle buffer;
    uint64_t bufferOffset = 0;
    uint64_t bufferRange = 0;
    ImageHandle image;
    SamplerHandle sampler;
};

struct GraphicsPipelineDesc
{
    ShaderModuleHandle vertexShader;
    ShaderModuleHandle fragmentShader;
    ShaderModuleHandle geometryShader;
    VertexInputStateDesc vertexInput;
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    RasterizerStateDesc rasterizer;
    DepthStencilStateDesc depthStencil;
    std::vector<BlendAttachmentDesc> blendAttachments;
    RenderTargetLayoutDesc renderTargetLayout;
    std::vector<DescriptorSetLayoutHandle> descriptorSetLayouts;
    uint32_t pushConstantSize = 0;
    std::string debugName;
};

struct ComputePipelineDesc
{
    ShaderModuleHandle computeShader;
    std::vector<DescriptorSetLayoutHandle> descriptorSetLayouts;
    uint32_t pushConstantSize = 0;
    std::string debugName;
};

struct RenderAttachmentDesc
{
    ImageHandle image;
    Format format = Format::Undefined;
    LoadOp loadOp = LoadOp::Clear;
    StoreOp storeOp = StoreOp::Store;
    ClearColor clearColor;
};

struct DepthStencilAttachmentDesc
{
    ImageHandle image;
    Format format = Format::Undefined;
    LoadOp depthLoadOp = LoadOp::Clear;
    StoreOp depthStoreOp = StoreOp::Store;
    ClearDepthStencil clearValue;
};

struct RenderPassBeginInfo
{
    std::vector<RenderAttachmentDesc> colorAttachments;
    bool hasDepthStencilAttachment = false;
    DepthStencilAttachmentDesc depthStencilAttachment;
    Rect2D renderArea;
};

struct RenderBackendCreateInfo
{
    void* nativeWindow = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    bool enableValidation = false;
    std::string applicationName = "AxisEngine";
    bool vsync = true;
};
}  // namespace rhi
