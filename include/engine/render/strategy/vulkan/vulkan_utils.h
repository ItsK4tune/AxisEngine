#pragma once
#include <core/logic/logger.h>
#include <render/rhi/rhi_types.h>
#include <vulkan/vulkan.h>
#include <sstream>
#if AXIS_HAS_VULKAN_BACKEND

namespace rhi
{
inline VkFormat ToVkFormat(Format format)
{
    switch (format)
    {
        case Format::R8:
            return VK_FORMAT_R8_UNORM;
        case Format::RG8:
            return VK_FORMAT_R8G8_UNORM;
        case Format::RGB8:
            return VK_FORMAT_R8G8B8_UNORM;
        case Format::RGBA8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::BGRA8:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::RGBA16F:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::R32UInt:
            return VK_FORMAT_R32_UINT;
        case Format::D24S8:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case Format::Depth32F:
            return VK_FORMAT_D32_SFLOAT;
        case Format::Depth24:
        case Format::Undefined:
        default:
            return VK_FORMAT_D32_SFLOAT;
    }
}

inline Format FromVkFormat(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_B8G8R8A8_UNORM:
            return Format::BGRA8;
        case VK_FORMAT_R8G8B8A8_UNORM:
            return Format::RGBA8;
        default:
            return Format::Undefined;
    }
}

inline VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage)
{
    VkBufferUsageFlags flags = 0;
    if (HasFlag(usage, BufferUsage::Vertex))
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (HasFlag(usage, BufferUsage::Index))
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (HasFlag(usage, BufferUsage::Uniform))
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (HasFlag(usage, BufferUsage::Storage))
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (HasFlag(usage, BufferUsage::TransferSrc))
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (HasFlag(usage, BufferUsage::TransferDst))
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (HasFlag(usage, BufferUsage::Indirect))
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    return flags ? flags : VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
}

inline VkImageUsageFlags ToVkImageUsage(ImageUsage usage)
{
    VkImageUsageFlags flags = 0;
    if (HasFlag(usage, ImageUsage::Sampled))
        flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (HasFlag(usage, ImageUsage::ColorAttachment))
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (HasFlag(usage, ImageUsage::DepthStencilAttachment))
        flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (HasFlag(usage, ImageUsage::Storage))
        flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (HasFlag(usage, ImageUsage::TransferSrc))
        flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (HasFlag(usage, ImageUsage::TransferDst))
        flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (HasFlag(usage, ImageUsage::Present))
        flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    return flags ? flags : VK_IMAGE_USAGE_SAMPLED_BIT;
}

inline VkMemoryPropertyFlags ToVkMemoryProperties(MemoryUsage usage)
{
    switch (usage)
    {
        case MemoryUsage::CpuToGpu:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        case MemoryUsage::GpuToCpu:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        case MemoryUsage::GpuOnly:
        default:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
}

inline VkShaderStageFlagBits ToVkShaderStageBit(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            return VK_SHADER_STAGE_ALL;
    }
}

inline VkShaderStageFlags ToVkShaderStages(ShaderStage stages)
{
    VkShaderStageFlags flags = 0;
    if (HasFlag(stages, ShaderStage::Vertex))
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    if (HasFlag(stages, ShaderStage::Fragment))
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (HasFlag(stages, ShaderStage::Geometry))
        flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    if (HasFlag(stages, ShaderStage::Compute))
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    return flags ? flags : VK_SHADER_STAGE_ALL;
}

inline VkDescriptorType ToVkDescriptorType(DescriptorType type)
{
    switch (type)
    {
        case DescriptorType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case DescriptorType::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case DescriptorType::SampledImage:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case DescriptorType::StorageImage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case DescriptorType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case DescriptorType::CombinedImageSampler:
        default:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }
}

inline VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PrimitiveTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::TriangleList:
        default:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

inline VkFormat ToVkVertexFormat(VertexFormat format)
{
    switch (format)
    {
        case VertexFormat::Float:
            return VK_FORMAT_R32_SFLOAT;
        case VertexFormat::Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case VertexFormat::Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case VertexFormat::Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case VertexFormat::Int:
            return VK_FORMAT_R32_SINT;
        case VertexFormat::Int2:
            return VK_FORMAT_R32G32_SINT;
        case VertexFormat::Int3:
            return VK_FORMAT_R32G32B32_SINT;
        case VertexFormat::Int4:
            return VK_FORMAT_R32G32B32A32_SINT;
        default:
            return VK_FORMAT_R32G32B32_SFLOAT;
    }
}

inline VkCompareOp ToVkCompareOp(CompareOp op)
{
    switch (op)
    {
        case CompareOp::Never:
            return VK_COMPARE_OP_NEVER;
        case CompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case CompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case CompareOp::LessOrEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case CompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::GreaterOrEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case CompareOp::Always:
        default:
            return VK_COMPARE_OP_ALWAYS;
    }
}

inline VkCullModeFlags ToVkCullMode(CullMode mode)
{
    switch (mode)
    {
        case CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:
            return VK_CULL_MODE_BACK_BIT;
        case CullMode::None:
        default:
            return VK_CULL_MODE_NONE;
    }
}

inline VkFrontFace ToVkFrontFace(FrontFace face)
{
    return face == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

inline VkPolygonMode ToVkPolygonMode(PolygonMode mode)
{
    switch (mode)
    {
        case PolygonMode::Line:
            return VK_POLYGON_MODE_LINE;
        case PolygonMode::Point:
            return VK_POLYGON_MODE_POINT;
        case PolygonMode::Fill:
        default:
            return VK_POLYGON_MODE_FILL;
    }
}

inline VkBlendFactor ToVkBlendFactor(BlendFactor factor)
{
    switch (factor)
    {
        case BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        default:
            return VK_BLEND_FACTOR_ONE;
    }
}

inline VkBlendOp ToVkBlendOp(BlendOp op)
{
    switch (op)
    {
        case BlendOp::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min:
            return VK_BLEND_OP_MIN;
        case BlendOp::Max:
            return VK_BLEND_OP_MAX;
        case BlendOp::Add:
        default:
            return VK_BLEND_OP_ADD;
    }
}

inline std::string RenderPassKey(const RenderTargetLayoutDesc& layout)
{
    std::ostringstream key;
    for (Format format : layout.colorFormats) key << static_cast<int>(format) << ",";
    key << "|d:" << static_cast<int>(layout.depthStencilFormat);
    return key.str();
}

inline VkFilter ToVkFilter(Filter filter)
{
    return filter == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

inline VkSamplerMipmapMode ToVkMipmapMode(MipmapMode mode)
{
    return mode == MipmapMode::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

inline VkSamplerAddressMode ToVkAddressMode(AddressMode mode)
{
    switch (mode)
    {
        case AddressMode::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case AddressMode::ClampToEdge:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case AddressMode::ClampToBorder:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case AddressMode::Repeat:
        default:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

inline VkImageAspectFlags GetAspectMask(Format format)
{
    if (format == Format::D24S8)
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    if (format == Format::Depth24 || format == Format::Depth32F)
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    return VK_IMAGE_ASPECT_COLOR_BIT;
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
        case Format::Depth24:
        case Format::Depth32F:
            return 4;
        case Format::RGBA16F:
            return 8;
        case Format::D24S8:
            return 4;
        case Format::Undefined:
        default:
            return 4;
    }
}

inline bool Check(VkResult result, const char* operation)
{
    if (result == VK_SUCCESS)
        return true;
    LOGGER_ERROR("VulkanRHI") << operation << " failed with VkResult " << static_cast<int>(result);
    return false;
}
}  // namespace rhi
#endif