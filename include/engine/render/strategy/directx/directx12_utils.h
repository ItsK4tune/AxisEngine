#pragma once
#include <render/rhi/rhi_types.h>
#include <render/strategy/directx/directx12_common.h>
#if AXIS_HAS_DIRECTX_BACKEND
#include <core/logic/logger.h>
#include <d3dcommon.h>
#include <d3d12sdklayers.h>
#include <iomanip>

namespace rhi
{
constexpr uint32_t kCbvSrvUavHeapCapacity = 4096;
constexpr uint32_t kSamplerHeapCapacity = 512;
constexpr uint32_t kRtvHeapCapacity = 1024;
constexpr uint32_t kDsvHeapCapacity = 512;
constexpr uint32_t kSwapchainBufferCount = 2;

inline bool Check(HRESULT result, const char* operation)
{
    if (SUCCEEDED(result))
        return true;
    LOGGER_ERROR("DirectX12RHI") << operation << " failed with HRESULT 0x" << std::hex
                                 << static_cast<unsigned long>(result);
    return false;
}

inline uint64_t AlignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

inline DXGI_FORMAT ToDxgiFormat(Format format)
{
    switch (format)
    {
        case Format::R8:
            return DXGI_FORMAT_R8_UNORM;
        case Format::RG8:
            return DXGI_FORMAT_R8G8_UNORM;
        case Format::RGB8:
        case Format::RGBA8:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::BGRA8:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case Format::RGBA16F:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case Format::R32UInt:
            return DXGI_FORMAT_R32_UINT;
        case Format::D24S8:
        case Format::Depth24:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case Format::Depth32F:
            return DXGI_FORMAT_D32_FLOAT;
        case Format::Undefined:
        default:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

inline DXGI_FORMAT ToShaderVisibleDxgiFormat(Format format)
{
    switch (format)
    {
        case Format::D24S8:
        case Format::Depth24:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case Format::Depth32F:
            return DXGI_FORMAT_R32_FLOAT;
        default:
            return ToDxgiFormat(format);
    }
}

inline DXGI_FORMAT ToResourceDxgiFormat(Format format, ImageUsage usage)
{
    const bool isDepth = format == Format::D24S8 || format == Format::Depth24 || format == Format::Depth32F;
    if (!HasFlag(usage, ImageUsage::Sampled) || !isDepth)
        return ToDxgiFormat(format);
    if (format == Format::Depth32F)
        return DXGI_FORMAT_R32_TYPELESS;
    return DXGI_FORMAT_R24G8_TYPELESS;
}

inline DXGI_FORMAT ToDepthStencilViewFormat(Format format)
{
    if (format == Format::Depth32F)
        return DXGI_FORMAT_D32_FLOAT;
    return DXGI_FORMAT_D24_UNORM_S8_UINT;
}

inline Format FromDxgiFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return Format::BGRA8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return Format::RGBA8;
        default:
            return Format::Undefined;
    }
}

inline uint32_t GetFormatByteSize(Format format)
{
    switch (format)
    {
        case Format::R8:
            return 1;
        case Format::RG8:
            return 2;
        case Format::RGB8:
            return 3;
        case Format::RGBA8:
        case Format::BGRA8:
        case Format::R32UInt:
        case Format::D24S8:
        case Format::Depth24:
        case Format::Depth32F:
            return 4;
        case Format::RGBA16F:
            return 8;
        case Format::Undefined:
        default:
            return 4;
    }
}

inline bool IsDepthFormat(Format format)
{
    return format == Format::D24S8 || format == Format::Depth24 || format == Format::Depth32F;
}

inline D3D12_RESOURCE_STATES InitialBufferState(MemoryUsage usage)
{
    switch (usage)
    {
        case MemoryUsage::CpuToGpu:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case MemoryUsage::GpuToCpu:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case MemoryUsage::GpuOnly:
        default:
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

inline D3D12_HEAP_TYPE ToHeapType(MemoryUsage usage)
{
    switch (usage)
    {
        case MemoryUsage::CpuToGpu:
            return D3D12_HEAP_TYPE_UPLOAD;
        case MemoryUsage::GpuToCpu:
            return D3D12_HEAP_TYPE_READBACK;
        case MemoryUsage::GpuOnly:
        default:
            return D3D12_HEAP_TYPE_DEFAULT;
    }
}

inline D3D12_FILTER ToD3DFilter(const SamplerDesc& desc)
{
    if (desc.minFilter == Filter::Nearest && desc.magFilter == Filter::Nearest &&
        desc.mipmapMode == MipmapMode::Nearest)
        return D3D12_FILTER_MIN_MAG_MIP_POINT;
    if (desc.minFilter == Filter::Nearest && desc.magFilter == Filter::Nearest)
        return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    if (desc.minFilter == Filter::Nearest)
        return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
    if (desc.magFilter == Filter::Nearest)
        return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
    return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
}

inline D3D12_TEXTURE_ADDRESS_MODE ToD3DAddressMode(AddressMode mode)
{
    switch (mode)
    {
        case AddressMode::MirroredRepeat:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case AddressMode::ClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case AddressMode::ClampToBorder:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case AddressMode::Repeat:
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

inline D3D12_PRIMITIVE_TOPOLOGY ToD3DTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::LineStrip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case PrimitiveTopology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case PrimitiveTopology::TriangleList:
        default:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3DTopologyType(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::PointList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopology::LineList:
        case PrimitiveTopology::LineStrip:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::TriangleList:
        case PrimitiveTopology::TriangleStrip:
        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

inline DXGI_FORMAT ToD3DVertexFormat(VertexFormat format)
{
    switch (format)
    {
        case VertexFormat::Float:
            return DXGI_FORMAT_R32_FLOAT;
        case VertexFormat::Float2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case VertexFormat::Float3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case VertexFormat::Float4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case VertexFormat::Int:
            return DXGI_FORMAT_R32_SINT;
        case VertexFormat::Int2:
            return DXGI_FORMAT_R32G32_SINT;
        case VertexFormat::Int3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case VertexFormat::Int4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        default:
            return DXGI_FORMAT_R32G32B32_FLOAT;
    }
}

inline D3D12_COMPARISON_FUNC ToD3DCompareOp(CompareOp op)
{
    switch (op)
    {
        case CompareOp::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case CompareOp::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case CompareOp::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case CompareOp::LessOrEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case CompareOp::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case CompareOp::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case CompareOp::GreaterOrEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case CompareOp::Always:
        default:
            return D3D12_COMPARISON_FUNC_ALWAYS;
    }
}

inline D3D12_CULL_MODE ToD3DCullMode(CullMode mode)
{
    switch (mode)
    {
        case CullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        case CullMode::Back:
            return D3D12_CULL_MODE_BACK;
        case CullMode::None:
        default:
            return D3D12_CULL_MODE_NONE;
    }
}

inline D3D12_FILL_MODE ToD3DFillMode(PolygonMode mode)
{
    return mode == PolygonMode::Line ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}

inline D3D12_BLEND ToD3DBlend(BlendFactor factor)
{
    switch (factor)
    {
        case BlendFactor::Zero:
            return D3D12_BLEND_ZERO;
        case BlendFactor::One:
            return D3D12_BLEND_ONE;
        case BlendFactor::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case BlendFactor::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case BlendFactor::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case BlendFactor::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        default:
            return D3D12_BLEND_ONE;
    }
}

inline D3D12_BLEND_OP ToD3DBlendOp(BlendOp op)
{
    switch (op)
    {
        case BlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case BlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case BlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        case BlendOp::Add:
        default:
            return D3D12_BLEND_OP_ADD;
    }
}

inline D3D12_SHADER_VISIBILITY ToShaderVisibility(ShaderStage stages)
{
    if (stages == ShaderStage::Vertex)
        return D3D12_SHADER_VISIBILITY_VERTEX;
    if (stages == ShaderStage::Fragment)
        return D3D12_SHADER_VISIBILITY_PIXEL;
    if (stages == ShaderStage::Compute)
        return D3D12_SHADER_VISIBILITY_ALL;
    return D3D12_SHADER_VISIBILITY_ALL;
}

inline D3D12_DESCRIPTOR_RANGE_TYPE ToDescriptorRangeType(DescriptorType type, bool samplerPart)
{
    if (samplerPart || type == DescriptorType::Sampler)
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    if (type == DescriptorType::UniformBuffer)
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    if (type == DescriptorType::StorageBuffer || type == DescriptorType::StorageImage)
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}

inline D3D12_RESOURCE_DIMENSION ToResourceDimension(ImageDimension dimension)
{
    return dimension == ImageDimension::Image3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D
                                                : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
}
}  // namespace rhi
#endif