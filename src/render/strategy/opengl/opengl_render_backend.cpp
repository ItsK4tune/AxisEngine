#include <render/strategy/opengl/opengl_render_backend.h>
#include <core/logic/logger.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>

namespace
{
uint32_t ToGLShaderStage(rhi::ShaderStage stage)
{
    switch (stage)
    {
        case rhi::ShaderStage::Vertex:
            return GL_VERTEX_SHADER;
        case rhi::ShaderStage::Fragment:
            return GL_FRAGMENT_SHADER;
        case rhi::ShaderStage::Geometry:
            return GL_GEOMETRY_SHADER;
        case rhi::ShaderStage::Compute:
            return GL_COMPUTE_SHADER;
        default:
            return 0;
    }
}

uint32_t ToGLBufferTarget(rhi::BufferUsage usage)
{
    if (rhi::HasFlag(usage, rhi::BufferUsage::Index))
        return GL_ELEMENT_ARRAY_BUFFER;
    if (rhi::HasFlag(usage, rhi::BufferUsage::Uniform))
        return GL_UNIFORM_BUFFER;
    if (rhi::HasFlag(usage, rhi::BufferUsage::Storage))
        return GL_SHADER_STORAGE_BUFFER;
    if (rhi::HasFlag(usage, rhi::BufferUsage::TransferSrc))
        return GL_COPY_READ_BUFFER;
    if (rhi::HasFlag(usage, rhi::BufferUsage::TransferDst))
        return GL_COPY_WRITE_BUFFER;
    return GL_ARRAY_BUFFER;
}

uint32_t ToGLBufferUsage(rhi::MemoryUsage usage)
{
    switch (usage)
    {
        case rhi::MemoryUsage::CpuToGpu:
            return GL_DYNAMIC_DRAW;
        case rhi::MemoryUsage::GpuToCpu:
            return GL_DYNAMIC_READ;
        case rhi::MemoryUsage::GpuOnly:
        default:
            return GL_STATIC_DRAW;
    }
}

uint32_t ToGLImageTarget(rhi::ImageDimension dimension)
{
    switch (dimension)
    {
        case rhi::ImageDimension::Image1D:
            return GL_TEXTURE_1D;
        case rhi::ImageDimension::Image3D:
            return GL_TEXTURE_3D;
        case rhi::ImageDimension::Cube:
            return GL_TEXTURE_CUBE_MAP;
        case rhi::ImageDimension::Image2D:
        default:
            return GL_TEXTURE_2D;
    }
}

uint32_t ToGLInternalFormat(rhi::Format format)
{
    switch (format)
    {
        case rhi::Format::R8:
            return GL_R8;
        case rhi::Format::RG8:
            return GL_RG8;
        case rhi::Format::RGB8:
            return GL_RGB8;
        case rhi::Format::RGBA8:
        case rhi::Format::BGRA8:
            return GL_RGBA8;
        case rhi::Format::RGBA16F:
            return GL_RGBA16F;
        case rhi::Format::R32UInt:
            return GL_R32UI;
        case rhi::Format::D24S8:
            return GL_DEPTH24_STENCIL8;
        case rhi::Format::Depth24:
            return GL_DEPTH_COMPONENT24;
        case rhi::Format::Depth32F:
            return GL_DEPTH_COMPONENT32F;
        case rhi::Format::Undefined:
        default:
            return GL_RGBA8;
    }
}

uint32_t ToGLExternalFormat(rhi::Format format)
{
    switch (format)
    {
        case rhi::Format::R8:
            return GL_RED;
        case rhi::Format::RG8:
            return GL_RG;
        case rhi::Format::RGB8:
            return GL_RGB;
        case rhi::Format::BGRA8:
            return GL_BGRA;
        case rhi::Format::R32UInt:
            return GL_RED_INTEGER;
        case rhi::Format::D24S8:
            return GL_DEPTH_STENCIL;
        case rhi::Format::Depth24:
        case rhi::Format::Depth32F:
            return GL_DEPTH_COMPONENT;
        case rhi::Format::RGBA8:
        case rhi::Format::RGBA16F:
        case rhi::Format::Undefined:
        default:
            return GL_RGBA;
    }
}

uint32_t ToGLDataType(rhi::Format format)
{
    switch (format)
    {
        case rhi::Format::RGBA16F:
        case rhi::Format::Depth24:
        case rhi::Format::Depth32F:
            return GL_FLOAT;
        case rhi::Format::R32UInt:
            return GL_UNSIGNED_INT;
        case rhi::Format::D24S8:
            return GL_UNSIGNED_INT_24_8;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

uint32_t ToGLFilter(rhi::Filter filter)
{
    return filter == rhi::Filter::Nearest ? GL_NEAREST : GL_LINEAR;
}

uint32_t ToGLMipmapFilter(rhi::Filter filter, rhi::MipmapMode mipmap)
{
    if (filter == rhi::Filter::Nearest)
        return mipmap == rhi::MipmapMode::Nearest ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
    return mipmap == rhi::MipmapMode::Nearest ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
}

uint32_t ToGLAddressMode(rhi::AddressMode mode)
{
    switch (mode)
    {
        case rhi::AddressMode::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
        case rhi::AddressMode::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case rhi::AddressMode::ClampToBorder:
            return GL_CLAMP_TO_BORDER;
        case rhi::AddressMode::Repeat:
        default:
            return GL_REPEAT;
    }
}

uint32_t ToGLTopology(rhi::PrimitiveTopology topology)
{
    switch (topology)
    {
        case rhi::PrimitiveTopology::PointList:
            return GL_POINTS;
        case rhi::PrimitiveTopology::LineList:
            return GL_LINES;
        case rhi::PrimitiveTopology::LineStrip:
            return GL_LINE_STRIP;
        case rhi::PrimitiveTopology::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case rhi::PrimitiveTopology::TriangleList:
        default:
            return GL_TRIANGLES;
    }
}

uint32_t ToGLIndexType(rhi::IndexType type)
{
    return type == rhi::IndexType::UInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

uint32_t IndexSize(rhi::IndexType type)
{
    return type == rhi::IndexType::UInt16 ? sizeof(uint16_t) : sizeof(uint32_t);
}

uint32_t ToGLCompareOp(rhi::CompareOp op)
{
    switch (op)
    {
        case rhi::CompareOp::Never:
            return GL_NEVER;
        case rhi::CompareOp::Less:
            return GL_LESS;
        case rhi::CompareOp::Equal:
            return GL_EQUAL;
        case rhi::CompareOp::LessOrEqual:
            return GL_LEQUAL;
        case rhi::CompareOp::Greater:
            return GL_GREATER;
        case rhi::CompareOp::NotEqual:
            return GL_NOTEQUAL;
        case rhi::CompareOp::GreaterOrEqual:
            return GL_GEQUAL;
        case rhi::CompareOp::Always:
        default:
            return GL_ALWAYS;
    }
}

uint32_t ToGLCullMode(rhi::CullMode mode)
{
    return mode == rhi::CullMode::Front ? GL_FRONT : GL_BACK;
}

uint32_t ToGLFrontFace(rhi::FrontFace face)
{
    return face == rhi::FrontFace::Clockwise ? GL_CW : GL_CCW;
}

uint32_t ToGLPolygonMode(rhi::PolygonMode mode)
{
    switch (mode)
    {
        case rhi::PolygonMode::Line:
            return GL_LINE;
        case rhi::PolygonMode::Point:
            return GL_POINT;
        case rhi::PolygonMode::Fill:
        default:
            return GL_FILL;
    }
}

uint32_t ToGLBlendFactor(rhi::BlendFactor factor)
{
    switch (factor)
    {
        case rhi::BlendFactor::Zero:
            return GL_ZERO;
        case rhi::BlendFactor::One:
            return GL_ONE;
        case rhi::BlendFactor::SrcColor:
            return GL_SRC_COLOR;
        case rhi::BlendFactor::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case rhi::BlendFactor::SrcAlpha:
            return GL_SRC_ALPHA;
        case rhi::BlendFactor::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case rhi::BlendFactor::DstColor:
            return GL_DST_COLOR;
        case rhi::BlendFactor::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case rhi::BlendFactor::DstAlpha:
            return GL_DST_ALPHA;
        case rhi::BlendFactor::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        default:
            return GL_ONE;
    }
}

uint32_t ToGLBlendOp(rhi::BlendOp op)
{
    switch (op)
    {
        case rhi::BlendOp::Subtract:
            return GL_FUNC_SUBTRACT;
        case rhi::BlendOp::ReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        case rhi::BlendOp::Min:
            return GL_MIN;
        case rhi::BlendOp::Max:
            return GL_MAX;
        case rhi::BlendOp::Add:
        default:
            return GL_FUNC_ADD;
    }
}

struct VertexFormatInfo
{
    uint32_t components = 3;
    uint32_t type = GL_FLOAT;
    bool integer = false;
};

VertexFormatInfo GetVertexFormatInfo(rhi::VertexFormat format)
{
    switch (format)
    {
        case rhi::VertexFormat::Float:
            return {1, GL_FLOAT, false};
        case rhi::VertexFormat::Float2:
            return {2, GL_FLOAT, false};
        case rhi::VertexFormat::Float3:
            return {3, GL_FLOAT, false};
        case rhi::VertexFormat::Float4:
            return {4, GL_FLOAT, false};
        case rhi::VertexFormat::Int:
            return {1, GL_INT, true};
        case rhi::VertexFormat::Int2:
            return {2, GL_INT, true};
        case rhi::VertexFormat::Int3:
            return {3, GL_INT, true};
        case rhi::VertexFormat::Int4:
            return {4, GL_INT, true};
        default:
            return {3, GL_FLOAT, false};
    }
}

bool CheckShaderCompile(uint32_t shader, const char* debugName)
{
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success)
        return true;

    char infoLog[2048] = {};
    glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
    LOGGER_ERROR("OpenGLRHI") << "Shader compile failed (" << (debugName ? debugName : "unnamed") << "): " << infoLog;
    return false;
}

bool CheckProgramLink(uint32_t program, const char* debugName)
{
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success)
        return true;

    char infoLog[2048] = {};
    glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
    LOGGER_ERROR("OpenGLRHI") << "Program link failed (" << (debugName ? debugName : "unnamed") << "): " << infoLog;
    return false;
}

void SetUniformMat4(uint32_t program, const char* name, const void* data, uint32_t size, uint32_t offset)
{
    if (!data || offset + sizeof(float) * 16 > size)
        return;

    const GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniformMatrix4fv(location, 1, GL_FALSE,
                           reinterpret_cast<const float*>(static_cast<const uint8_t*>(data) + offset));
}

void SetUniformVec4(uint32_t program, const char* name, const void* data, uint32_t size, uint32_t offset)
{
    if (!data || offset + sizeof(float) * 4 > size)
        return;

    const GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform4fv(location, 1, reinterpret_cast<const float*>(static_cast<const uint8_t*>(data) + offset));
}

void SetUniformFloat(uint32_t program, const char* name, const void* data, uint32_t size, uint32_t offset)
{
    if (!data || offset + sizeof(float) > size)
        return;

    const GLint location = glGetUniformLocation(program, name);
    if (location >= 0)
        glUniform1f(location, *reinterpret_cast<const float*>(static_cast<const uint8_t*>(data) + offset));
}

void ApplyPushConstants(uint32_t program, const void* data, uint32_t size)
{
    constexpr uint32_t kMat4 = sizeof(float) * 16;
    constexpr uint32_t kVec4 = sizeof(float) * 4;

    SetUniformMat4(program, "u_Mvp", data, size, 0);
    SetUniformVec4(program, "u_Color", data, size, kMat4);
    SetUniformFloat(program, "u_Intensity", data, size, kMat4);

    SetUniformMat4(program, "u_Model", data, size, kMat4);
    SetUniformVec4(program, "u_Color", data, size, kMat4 * 2);
    SetUniformVec4(program, "u_TintColor", data, size, kMat4 * 2);
    SetUniformVec4(program, "u_PbrParams", data, size, kMat4 * 2 + kVec4);
    SetUniformVec4(program, "u_DecalParams", data, size, kMat4 * 2 + kVec4);
    SetUniformVec4(program, "u_CameraPos", data, size, kMat4 * 2 + kVec4 * 2);
    SetUniformVec4(program, "u_DirLightDir", data, size, kMat4 * 2 + kVec4 * 3);
    SetUniformVec4(program, "u_DirLightColor", data, size, kMat4 * 2 + kVec4 * 4);
}
}  // namespace

OpenGLCommandList::OpenGLCommandList(OpenGLRenderDevice& device) : m_Device(device)
{
}

OpenGLCommandList::~OpenGLCommandList()
{
    if (m_RenderFramebuffer != 0)
    {
        glDeleteFramebuffers(1, &m_RenderFramebuffer);
        m_RenderFramebuffer = 0;
    }
}

void OpenGLCommandList::Begin()
{
    m_IsRecording = true;
    m_CurrentPipeline = {};
    m_VertexBuffers.clear();
    if (m_Device.m_DefaultVertexArray != 0)
        glBindVertexArray(m_Device.m_DefaultVertexArray);
}

void OpenGLCommandList::End()
{
    m_IsRecording = false;
}

void OpenGLCommandList::BeginRendering(const rhi::RenderPassBeginInfo& beginInfo)
{
    const bool renderToDefault = beginInfo.colorAttachments.empty() || !beginInfo.colorAttachments.front().image;
    if (renderToDefault)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else
    {
        if (m_RenderFramebuffer == 0)
            glGenFramebuffers(1, &m_RenderFramebuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, m_RenderFramebuffer);
        std::vector<GLenum> drawBuffers;
        drawBuffers.reserve(beginInfo.colorAttachments.size());
        for (size_t i = 0; i < beginInfo.colorAttachments.size(); ++i)
        {
            const auto* image = m_Device.GetImage(beginInfo.colorAttachments[i].image);
            if (!image)
                continue;
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), image->target,
                                   image->object, 0);
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        }

        if (beginInfo.hasDepthStencilAttachment && beginInfo.depthStencilAttachment.image)
        {
            if (const auto* image = m_Device.GetImage(beginInfo.depthStencilAttachment.image))
            {
                GLenum attachment =
                    image->desc.format == rhi::Format::D24S8 ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, image->target, image->object, 0);
            }
        }

        if (!drawBuffers.empty())
            glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
    }

    SetScissor(beginInfo.renderArea);
    SetViewport(rhi::Viewport{static_cast<float>(beginInfo.renderArea.x), static_cast<float>(beginInfo.renderArea.y),
                              static_cast<float>(beginInfo.renderArea.width),
                              static_cast<float>(beginInfo.renderArea.height), 0.0f, 1.0f});

    for (size_t i = 0; i < beginInfo.colorAttachments.size(); ++i)
    {
        const auto& attachment = beginInfo.colorAttachments[i];
        if (attachment.loadOp != rhi::LoadOp::Clear)
            continue;

        const float color[4] = {attachment.clearColor.r, attachment.clearColor.g, attachment.clearColor.b,
                                attachment.clearColor.a};
        if (renderToDefault)
        {
            glClearColor(color[0], color[1], color[2], color[3]);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        else
        {
            glClearBufferfv(GL_COLOR, static_cast<GLint>(i), color);
        }
    }

    if (beginInfo.hasDepthStencilAttachment && beginInfo.depthStencilAttachment.depthLoadOp == rhi::LoadOp::Clear)
    {
        const auto clear = beginInfo.depthStencilAttachment.clearValue;
        if (beginInfo.depthStencilAttachment.format == rhi::Format::D24S8)
            glClearBufferfi(GL_DEPTH_STENCIL, 0, clear.depth, static_cast<GLint>(clear.stencil));
        else
            glClearBufferfv(GL_DEPTH, 0, &clear.depth);
    }
}

void OpenGLCommandList::EndRendering()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLCommandList::SetViewport(const rhi::Viewport& viewport)
{
    glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y), static_cast<GLsizei>(viewport.width),
               static_cast<GLsizei>(viewport.height));
    glDepthRange(viewport.minDepth, viewport.maxDepth);
}

void OpenGLCommandList::SetScissor(const rhi::Rect2D& scissor)
{
    glScissor(scissor.x, scissor.y, static_cast<GLsizei>(scissor.width), static_cast<GLsizei>(scissor.height));
}

void OpenGLCommandList::BindPipeline(rhi::PipelineHandle pipeline)
{
    m_CurrentPipeline = pipeline;
    const auto* pipelineResource = m_Device.GetPipeline(pipeline);
    if (!pipelineResource)
        return;

    glUseProgram(pipelineResource->program);
    ApplyPipelineState();
    ApplyVertexInput();
}

void OpenGLCommandList::BindDescriptorSet(uint32_t setIndex, rhi::DescriptorSetHandle descriptorSet)
{
    (void)setIndex;
    const auto* set = m_Device.GetDescriptorSet(descriptorSet);
    if (!set)
        return;

    for (const auto& update : set->updates) BindDescriptorUpdate(update);
}

void OpenGLCommandList::BindVertexBuffer(uint32_t binding, rhi::BufferHandle buffer, uint64_t offset)
{
    m_VertexBuffers[binding] = BoundVertexBuffer{buffer, offset};
    ApplyVertexInput();
}

void OpenGLCommandList::BindIndexBuffer(rhi::BufferHandle buffer, rhi::IndexType indexType, uint64_t offset)
{
    if (const auto* bufferResource = m_Device.GetBuffer(buffer))
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferResource->object);
        m_IndexType = indexType;
        m_IndexOffset = offset;
    }
}

void OpenGLCommandList::PushConstants(rhi::ShaderStage stages, const void* data, uint32_t size, uint32_t offset)
{
    (void)stages;
    (void)offset;
    const auto* pipeline = m_Device.GetPipeline(m_CurrentPipeline);
    if (!pipeline || pipeline->program == 0 || !data || size == 0)
        return;

    glUseProgram(pipeline->program);
    ApplyPushConstants(pipeline->program, data, size);
}

void OpenGLCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    const auto* pipeline = m_Device.GetPipeline(m_CurrentPipeline);
    GLenum topology = pipeline ? ToGLTopology(pipeline->graphicsDesc.topology) : GL_TRIANGLES;
    glDrawArraysInstancedBaseInstance(topology, static_cast<GLint>(firstVertex), static_cast<GLsizei>(vertexCount),
                                      static_cast<GLsizei>(instanceCount), firstInstance);
}

void OpenGLCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                    int32_t vertexOffset, uint32_t firstInstance)
{
    const auto* pipeline = m_Device.GetPipeline(m_CurrentPipeline);
    GLenum topology = pipeline ? ToGLTopology(pipeline->graphicsDesc.topology) : GL_TRIANGLES;
    const auto byteOffset = m_IndexOffset + static_cast<uint64_t>(firstIndex) * IndexSize(m_IndexType);
    glDrawElementsInstancedBaseVertexBaseInstance(topology, static_cast<GLsizei>(indexCount),
                                                  ToGLIndexType(m_IndexType), reinterpret_cast<const void*>(byteOffset),
                                                  static_cast<GLsizei>(instanceCount), vertexOffset, firstInstance);
}

void OpenGLCommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    glDispatchCompute(groupCountX, groupCountY, groupCountZ);
}

void OpenGLCommandList::ApplyPipelineState()
{
    const auto* pipeline = m_Device.GetPipeline(m_CurrentPipeline);
    if (!pipeline || pipeline->compute)
        return;

    const auto& desc = pipeline->graphicsDesc;

    if (desc.rasterizer.cullMode == rhi::CullMode::None)
        glDisable(GL_CULL_FACE);
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(ToGLCullMode(desc.rasterizer.cullMode));
    }

    glFrontFace(ToGLFrontFace(desc.rasterizer.frontFace));
    glPolygonMode(GL_FRONT_AND_BACK, ToGLPolygonMode(desc.rasterizer.polygonMode));
    glLineWidth(desc.rasterizer.lineWidth);

    if (desc.depthStencil.depthTestEnable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    glDepthMask(desc.depthStencil.depthWriteEnable ? GL_TRUE : GL_FALSE);
    glDepthFunc(ToGLCompareOp(desc.depthStencil.depthCompare));

    if (!desc.blendAttachments.empty() && desc.blendAttachments.front().blendEnable)
    {
        const auto& blend = desc.blendAttachments.front();
        glEnable(GL_BLEND);
        glBlendFuncSeparate(ToGLBlendFactor(blend.srcColorBlendFactor), ToGLBlendFactor(blend.dstColorBlendFactor),
                            ToGLBlendFactor(blend.srcAlphaBlendFactor), ToGLBlendFactor(blend.dstAlphaBlendFactor));
        glBlendEquationSeparate(ToGLBlendOp(blend.colorBlendOp), ToGLBlendOp(blend.alphaBlendOp));
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void OpenGLCommandList::ApplyVertexInput()
{
    const auto* pipeline = m_Device.GetPipeline(m_CurrentPipeline);
    if (!pipeline || pipeline->compute)
        return;

    const auto& input = pipeline->graphicsDesc.vertexInput;
    for (const auto& attribute : input.attributes)
    {
        auto boundIt = m_VertexBuffers.find(attribute.binding);
        if (boundIt == m_VertexBuffers.end())
            continue;

        const auto* buffer = m_Device.GetBuffer(boundIt->second.buffer);
        if (!buffer)
            continue;

        const auto bindingIt =
            std::find_if(input.bindings.begin(), input.bindings.end(),
                         [&](const rhi::VertexBindingDesc& binding) { return binding.binding == attribute.binding; });
        if (bindingIt == input.bindings.end())
            continue;

        const auto format = GetVertexFormatInfo(attribute.format);
        const auto pointerOffset = boundIt->second.offset + attribute.offset;
        glBindBuffer(GL_ARRAY_BUFFER, buffer->object);
        glEnableVertexAttribArray(attribute.location);
        if (format.integer)
        {
            glVertexAttribIPointer(attribute.location, static_cast<GLint>(format.components), format.type,
                                   static_cast<GLsizei>(bindingIt->stride),
                                   reinterpret_cast<const void*>(pointerOffset));
        }
        else
        {
            glVertexAttribPointer(attribute.location, static_cast<GLint>(format.components), format.type, GL_FALSE,
                                  static_cast<GLsizei>(bindingIt->stride),
                                  reinterpret_cast<const void*>(pointerOffset));
        }
        glVertexAttribDivisor(attribute.location, bindingIt->inputRate == rhi::VertexInputRate::PerInstance ? 1u : 0u);
    }
}

void OpenGLCommandList::BindDescriptorUpdate(const rhi::DescriptorUpdate& update)
{
    const GLuint slot = update.binding + update.arrayElement;
    switch (update.type)
    {
        case rhi::DescriptorType::UniformBuffer:
        case rhi::DescriptorType::StorageBuffer: {
            const auto* buffer = m_Device.GetBuffer(update.buffer);
            if (!buffer)
                return;
            GLenum target =
                update.type == rhi::DescriptorType::UniformBuffer ? GL_UNIFORM_BUFFER : GL_SHADER_STORAGE_BUFFER;
            if (update.bufferRange > 0)
            {
                glBindBufferRange(target, slot, buffer->object, static_cast<GLintptr>(update.bufferOffset),
                                  static_cast<GLsizeiptr>(update.bufferRange));
            }
            else
            {
                glBindBufferBase(target, slot, buffer->object);
            }
            break;
        }
        case rhi::DescriptorType::SampledImage:
        case rhi::DescriptorType::StorageImage:
        case rhi::DescriptorType::CombinedImageSampler: {
            const auto* image = m_Device.GetImage(update.image);
            if (!image)
                return;
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(image->target, image->object);
            if (update.sampler)
            {
                const auto* sampler = m_Device.GetSampler(update.sampler);
                if (sampler)
                    glBindSampler(slot, sampler->object);
            }
            break;
        }
        case rhi::DescriptorType::Sampler: {
            const auto* sampler = m_Device.GetSampler(update.sampler);
            if (sampler)
                glBindSampler(slot, sampler->object);
            break;
        }
    }
}

void OpenGLSwapchain::Resize(uint32_t width, uint32_t height)
{
    m_Extent = {width, height};
}

rhi::Extent2D OpenGLSwapchain::GetExtent() const
{
    return m_Extent;
}

rhi::Format OpenGLSwapchain::GetBackBufferFormat() const
{
    return m_BackBufferFormat;
}

rhi::ImageHandle OpenGLSwapchain::GetCurrentBackBuffer() const
{
    return {};
}

OpenGLRenderDevice::OpenGLRenderDevice() : m_CommandList(*this)
{
}

OpenGLRenderDevice::~OpenGLRenderDevice()
{
    Shutdown();
}

void OpenGLRenderDevice::Initialize(uint32_t defaultVertexArray)
{
    m_DefaultVertexArray = defaultVertexArray;
}

void OpenGLRenderDevice::Shutdown()
{
    for (auto& [_, pipeline] : m_Pipelines) glDeleteProgram(pipeline.program);
    for (auto& [_, shader] : m_ShaderModules) glDeleteShader(shader.object);
    for (auto& [_, sampler] : m_Samplers) glDeleteSamplers(1, &sampler.object);
    for (auto& [_, image] : m_Images) glDeleteTextures(1, &image.object);
    for (auto& [_, buffer] : m_Buffers) glDeleteBuffers(1, &buffer.object);

    m_Pipelines.clear();
    m_ShaderModules.clear();
    m_Samplers.clear();
    m_Images.clear();
    m_Buffers.clear();
    m_DescriptorSets.clear();
    m_DescriptorSetLayouts.clear();
}

rhi::BufferHandle OpenGLRenderDevice::CreateBuffer(const rhi::BufferDesc& desc, const void* initialData)
{
    GLuint object = 0;
    glGenBuffers(1, &object);
    const auto target = ToGLBufferTarget(desc.usage);
    glBindBuffer(target, object);
    glBufferData(target, static_cast<GLsizeiptr>(desc.size), initialData, ToGLBufferUsage(desc.memoryUsage));

    const uint32_t id = AllocateHandle();
    m_Buffers[id] = BufferResource{object, target, desc.size, desc.usage};
    return rhi::BufferHandle{id};
}

void OpenGLRenderDevice::UpdateBuffer(rhi::BufferHandle handle, uint64_t offset, uint64_t size, const void* data)
{
    auto* buffer = const_cast<BufferResource*>(GetBuffer(handle));
    if (!buffer || !data)
        return;

    glBindBuffer(buffer->target, buffer->object);
    glBufferSubData(buffer->target, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
}

void OpenGLRenderDevice::DestroyBuffer(rhi::BufferHandle handle)
{
    auto it = m_Buffers.find(handle.id);
    if (it == m_Buffers.end())
        return;
    glDeleteBuffers(1, &it->second.object);
    m_Buffers.erase(it);
}

rhi::ImageHandle OpenGLRenderDevice::CreateImage(const rhi::ImageDesc& desc, const void* initialData)
{
    GLuint object = 0;
    glGenTextures(1, &object);
    const auto target = ToGLImageTarget(desc.dimension);
    glBindTexture(target, object);

    if (desc.dimension == rhi::ImageDimension::Image3D)
    {
        glTexImage3D(target, 0, ToGLInternalFormat(desc.format), static_cast<GLsizei>(desc.width),
                     static_cast<GLsizei>(desc.height), static_cast<GLsizei>(desc.depth), 0,
                     ToGLExternalFormat(desc.format), ToGLDataType(desc.format), initialData);
    }
    else if (desc.dimension == rhi::ImageDimension::Cube)
    {
        for (uint32_t face = 0; face < 6; ++face)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, ToGLInternalFormat(desc.format),
                         static_cast<GLsizei>(desc.width), static_cast<GLsizei>(desc.height), 0,
                         ToGLExternalFormat(desc.format), ToGLDataType(desc.format), nullptr);
        }
    }
    else
    {
        glTexImage2D(target, 0, ToGLInternalFormat(desc.format), static_cast<GLsizei>(desc.width),
                     static_cast<GLsizei>(desc.height), 0, ToGLExternalFormat(desc.format), ToGLDataType(desc.format),
                     initialData);
    }

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);

    const uint32_t id = AllocateHandle();
    m_Images[id] = ImageResource{object, target, desc};
    return rhi::ImageHandle{id};
}

void OpenGLRenderDevice::DestroyImage(rhi::ImageHandle handle)
{
    auto it = m_Images.find(handle.id);
    if (it == m_Images.end())
        return;
    glDeleteTextures(1, &it->second.object);
    m_Images.erase(it);
}

rhi::SamplerHandle OpenGLRenderDevice::CreateSampler(const rhi::SamplerDesc& desc)
{
    GLuint object = 0;
    glGenSamplers(1, &object);
    glSamplerParameteri(object, GL_TEXTURE_MIN_FILTER, ToGLMipmapFilter(desc.minFilter, desc.mipmapMode));
    glSamplerParameteri(object, GL_TEXTURE_MAG_FILTER, ToGLFilter(desc.magFilter));
    glSamplerParameteri(object, GL_TEXTURE_WRAP_S, ToGLAddressMode(desc.addressU));
    glSamplerParameteri(object, GL_TEXTURE_WRAP_T, ToGLAddressMode(desc.addressV));
    glSamplerParameteri(object, GL_TEXTURE_WRAP_R, ToGLAddressMode(desc.addressW));
#if defined(GL_TEXTURE_MAX_ANISOTROPY)
    if (desc.maxAnisotropy > 1.0f)
        glSamplerParameterf(object, GL_TEXTURE_MAX_ANISOTROPY, desc.maxAnisotropy);
#elif defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
    if (desc.maxAnisotropy > 1.0f)
        glSamplerParameterf(object, GL_TEXTURE_MAX_ANISOTROPY_EXT, desc.maxAnisotropy);
#endif

    const uint32_t id = AllocateHandle();
    m_Samplers[id] = SamplerResource{object};
    return rhi::SamplerHandle{id};
}

void OpenGLRenderDevice::DestroySampler(rhi::SamplerHandle handle)
{
    auto it = m_Samplers.find(handle.id);
    if (it == m_Samplers.end())
        return;
    glDeleteSamplers(1, &it->second.object);
    m_Samplers.erase(it);
}

rhi::ShaderModuleHandle OpenGLRenderDevice::CreateShaderModule(const rhi::ShaderModuleDesc& desc)
{
    if (desc.sourceType != rhi::ShaderSourceType::SourceText || !desc.data || desc.size == 0)
    {
        LOGGER_ERROR("OpenGLRHI") << "OpenGL shader modules require GLSL source text.";
        return {};
    }

    const GLuint shader = glCreateShader(ToGLShaderStage(desc.stage));
    const GLchar* source = static_cast<const GLchar*>(desc.data);
    GLint length = static_cast<GLint>(desc.size);
    glShaderSource(shader, 1, &source, &length);
    glCompileShader(shader);
    if (!CheckShaderCompile(shader, desc.debugName.c_str()))
    {
        glDeleteShader(shader);
        return {};
    }

    const uint32_t id = AllocateHandle();
    m_ShaderModules[id] = ShaderModuleResource{shader, desc.stage};
    return rhi::ShaderModuleHandle{id};
}

void OpenGLRenderDevice::DestroyShaderModule(rhi::ShaderModuleHandle handle)
{
    auto it = m_ShaderModules.find(handle.id);
    if (it == m_ShaderModules.end())
        return;
    glDeleteShader(it->second.object);
    m_ShaderModules.erase(it);
}

rhi::DescriptorSetLayoutHandle OpenGLRenderDevice::CreateDescriptorSetLayout(const rhi::DescriptorSetLayoutDesc& desc)
{
    const uint32_t id = AllocateHandle();
    m_DescriptorSetLayouts[id] = DescriptorSetLayoutResource{desc};
    return rhi::DescriptorSetLayoutHandle{id};
}

void OpenGLRenderDevice::DestroyDescriptorSetLayout(rhi::DescriptorSetLayoutHandle handle)
{
    m_DescriptorSetLayouts.erase(handle.id);
}

rhi::DescriptorSetHandle OpenGLRenderDevice::CreateDescriptorSet(rhi::DescriptorSetLayoutHandle layout)
{
    if (m_DescriptorSetLayouts.find(layout.id) == m_DescriptorSetLayouts.end())
        return {};

    const uint32_t id = AllocateHandle();
    m_DescriptorSets[id] = DescriptorSetResource{layout, {}};
    return rhi::DescriptorSetHandle{id};
}

void OpenGLRenderDevice::UpdateDescriptorSet(rhi::DescriptorSetHandle set, const rhi::DescriptorUpdate* updates,
                                             uint32_t count)
{
    auto it = m_DescriptorSets.find(set.id);
    if (it == m_DescriptorSets.end() || !updates)
        return;

    for (uint32_t i = 0; i < count; ++i)
    {
        auto& existing = it->second.updates;
        auto existingIt = std::find_if(existing.begin(), existing.end(), [&](const rhi::DescriptorUpdate& update) {
            return update.binding == updates[i].binding && update.arrayElement == updates[i].arrayElement;
        });
        if (existingIt == existing.end())
            existing.push_back(updates[i]);
        else
            *existingIt = updates[i];
    }
}

void OpenGLRenderDevice::DestroyDescriptorSet(rhi::DescriptorSetHandle handle)
{
    m_DescriptorSets.erase(handle.id);
}

rhi::PipelineHandle OpenGLRenderDevice::CreateGraphicsPipeline(const rhi::GraphicsPipelineDesc& desc)
{
    const auto* vertexShader = GetShaderModule(desc.vertexShader);
    const auto* fragmentShader = GetShaderModule(desc.fragmentShader);
    if (!vertexShader || !fragmentShader)
        return {};

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader->object);
    glAttachShader(program, fragmentShader->object);
    if (const auto* geometryShader = GetShaderModule(desc.geometryShader))
        glAttachShader(program, geometryShader->object);
    glLinkProgram(program);

    if (!CheckProgramLink(program, desc.debugName.c_str()))
    {
        glDeleteProgram(program);
        return {};
    }

    const uint32_t id = AllocateHandle();
    m_Pipelines[id] = PipelineResource{program, false, desc, {}};
    return rhi::PipelineHandle{id};
}

rhi::PipelineHandle OpenGLRenderDevice::CreateComputePipeline(const rhi::ComputePipelineDesc& desc)
{
    const auto* computeShader = GetShaderModule(desc.computeShader);
    if (!computeShader)
        return {};

    GLuint program = glCreateProgram();
    glAttachShader(program, computeShader->object);
    glLinkProgram(program);

    if (!CheckProgramLink(program, desc.debugName.c_str()))
    {
        glDeleteProgram(program);
        return {};
    }

    const uint32_t id = AllocateHandle();
    m_Pipelines[id] = PipelineResource{program, true, {}, desc};
    return rhi::PipelineHandle{id};
}

void OpenGLRenderDevice::DestroyPipeline(rhi::PipelineHandle handle)
{
    auto it = m_Pipelines.find(handle.id);
    if (it == m_Pipelines.end())
        return;
    glDeleteProgram(it->second.program);
    m_Pipelines.erase(it);
}

rhi::ICommandList& OpenGLRenderDevice::BeginCommandList(rhi::CommandQueueType queue)
{
    (void)queue;
    m_CommandList.Begin();
    return m_CommandList;
}

void OpenGLRenderDevice::Submit(rhi::ICommandList& commandList)
{
    commandList.End();
}

void OpenGLRenderDevice::WaitIdle()
{
    glFinish();
}

rhi::BackendType OpenGLRenderDevice::GetBackendType() const
{
    return rhi::BackendType::OpenGL;
}

const char* OpenGLRenderDevice::GetName() const
{
    return "OpenGL";
}

uint32_t OpenGLRenderDevice::AllocateHandle()
{
    return m_NextHandle++;
}

const OpenGLRenderDevice::BufferResource* OpenGLRenderDevice::GetBuffer(rhi::BufferHandle handle) const
{
    auto it = m_Buffers.find(handle.id);
    return it == m_Buffers.end() ? nullptr : &it->second;
}

const OpenGLRenderDevice::ImageResource* OpenGLRenderDevice::GetImage(rhi::ImageHandle handle) const
{
    auto it = m_Images.find(handle.id);
    return it == m_Images.end() ? nullptr : &it->second;
}

const OpenGLRenderDevice::SamplerResource* OpenGLRenderDevice::GetSampler(rhi::SamplerHandle handle) const
{
    auto it = m_Samplers.find(handle.id);
    return it == m_Samplers.end() ? nullptr : &it->second;
}

const OpenGLRenderDevice::ShaderModuleResource* OpenGLRenderDevice::GetShaderModule(
    rhi::ShaderModuleHandle handle) const
{
    auto it = m_ShaderModules.find(handle.id);
    return it == m_ShaderModules.end() ? nullptr : &it->second;
}

const OpenGLRenderDevice::DescriptorSetResource* OpenGLRenderDevice::GetDescriptorSet(
    rhi::DescriptorSetHandle handle) const
{
    auto it = m_DescriptorSets.find(handle.id);
    return it == m_DescriptorSets.end() ? nullptr : &it->second;
}

const OpenGLRenderDevice::PipelineResource* OpenGLRenderDevice::GetPipeline(rhi::PipelineHandle handle) const
{
    auto it = m_Pipelines.find(handle.id);
    return it == m_Pipelines.end() ? nullptr : &it->second;
}

OpenGLRenderBackend::OpenGLRenderBackend()
{
}

OpenGLRenderBackend::~OpenGLRenderBackend()
{
    Shutdown();
}

bool OpenGLRenderBackend::Initialize(const rhi::RenderBackendCreateInfo& createInfo)
{
    if (m_Initialized)
        return true;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOGGER_ERROR("OpenGLRHI") << "Failed to initialize GLAD";
        return false;
    }

    LOGGER_INFO("OpenGLRHI") << "OpenGL RHI initialized: " << glGetString(GL_VERSION);

    glGenVertexArrays(1, &m_DefaultVertexArray);
    glBindVertexArray(m_DefaultVertexArray);

    m_Device.Initialize(m_DefaultVertexArray);
    m_Swapchain.Resize(createInfo.width, createInfo.height);
    m_Initialized = true;
    return true;
}

void OpenGLRenderBackend::Shutdown()
{
    if (!m_Initialized)
        return;

    m_Device.Shutdown();
    if (m_DefaultVertexArray != 0)
    {
        glDeleteVertexArrays(1, &m_DefaultVertexArray);
        m_DefaultVertexArray = 0;
    }
    m_Initialized = false;
}

bool OpenGLRenderBackend::BeginFrame()
{
    return m_Initialized;
}

void OpenGLRenderBackend::EndFrame()
{
}

void OpenGLRenderBackend::OnResize(uint32_t width, uint32_t height)
{
    m_Swapchain.Resize(width, height);
}

rhi::IRenderDevice& OpenGLRenderBackend::GetDevice()
{
    return m_Device;
}

rhi::ISwapchain& OpenGLRenderBackend::GetSwapchain()
{
    return m_Swapchain;
}

rhi::BackendType OpenGLRenderBackend::GetBackendType() const
{
    return rhi::BackendType::OpenGL;
}

const char* OpenGLRenderBackend::GetName() const
{
    return "OpenGL";
}
