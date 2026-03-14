#pragma once

#include <core/logic/logger.h>
#include <glad/glad.h>
#include <render/type/graphics_types.h>

class GLTranslator
{
public:
    static GLenum ToGL(Primitive p) {
        switch (p) {
            case Primitive::Points: return GL_POINTS;
            case Primitive::Lines: return GL_LINES;
            case Primitive::LineLoop: return GL_LINE_LOOP;
            case Primitive::LineStrip: return GL_LINE_STRIP;
            case Primitive::Triangles: return GL_TRIANGLES;
            case Primitive::TriangleStrip: return GL_TRIANGLE_STRIP;
            case Primitive::TriangleFan: return GL_TRIANGLE_FAN;
            default: return GL_TRIANGLES;
        }
    }

    static GLenum ToGL(DataType t) {
        switch (t) {
            case DataType::Byte: return GL_BYTE;
            case DataType::UnsignedByte: return GL_UNSIGNED_BYTE;
            case DataType::Short: return GL_SHORT;
            case DataType::UnsignedShort: return GL_UNSIGNED_SHORT;
            case DataType::Int: return GL_INT;
            case DataType::UnsignedInt: return GL_UNSIGNED_INT;
            case DataType::Float: return GL_FLOAT;
            case DataType::Double: return GL_DOUBLE;
            default: return GL_FLOAT;
        }
    }

    static GLenum ToGL(BufferType t) {
        switch (t) {
            case BufferType::ArrayBuffer: return GL_ARRAY_BUFFER;
            case BufferType::ElementArrayBuffer: return GL_ELEMENT_ARRAY_BUFFER;
            case BufferType::UniformBuffer: return GL_UNIFORM_BUFFER;
            case BufferType::ShaderStorageBuffer: return GL_SHADER_STORAGE_BUFFER;
            default: return GL_ARRAY_BUFFER;
        }
    }

    static GLenum ToGL(BufferUsage u) {
        switch (u) {
            case BufferUsage::StreamDraw: return GL_STREAM_DRAW;
            case BufferUsage::StreamRead: return GL_STREAM_READ;
            case BufferUsage::StreamCopy: return GL_STREAM_COPY;
            case BufferUsage::StaticDraw: return GL_STATIC_DRAW;
            case BufferUsage::StaticRead: return GL_STATIC_READ;
            case BufferUsage::StaticCopy: return GL_STATIC_COPY;
            case BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
            case BufferUsage::DynamicRead: return GL_DYNAMIC_READ;
            case BufferUsage::DynamicCopy: return GL_DYNAMIC_COPY;
            default: return GL_STATIC_DRAW;
        }
    }

    static GLenum ToGL(TextureType t) {
        switch (t) {
            case TextureType::Texture1D: return GL_TEXTURE_1D;
            case TextureType::Texture2D: return GL_TEXTURE_2D;
            case TextureType::TextureCubeMap: return GL_TEXTURE_CUBE_MAP;
            case TextureType::Texture3D: return GL_TEXTURE_3D;
            case TextureType::CubeMapPositiveX: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            case TextureType::CubeMapNegativeX: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
            case TextureType::CubeMapPositiveY: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
            case TextureType::CubeMapNegativeY: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
            case TextureType::CubeMapPositiveZ: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
            case TextureType::CubeMapNegativeZ: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
            default: return GL_TEXTURE_2D;
        }
    }

    static GLenum ToGL(TextureFormat f) {
        switch (f) {
            case TextureFormat::Red: return GL_RED;
            case TextureFormat::RG: return GL_RG;
            case TextureFormat::RGB: return GL_RGB;
            case TextureFormat::RGBA: return GL_RGBA;
            case TextureFormat::DepthComponent: return GL_DEPTH_COMPONENT;
            case TextureFormat::DepthStencil: return GL_DEPTH_STENCIL;
            case TextureFormat::Red_Integer: return GL_RED_INTEGER;
            default: return GL_RGBA;
        }
    }

    static GLenum ToGL(InternalFormat f) {
        switch (f) {
            case InternalFormat::R8: return GL_R8;
            case InternalFormat::RGB8: return GL_RGB8;
            case InternalFormat::RGBA8: return GL_RGBA8;
            case InternalFormat::RGBA16F: return GL_RGBA16F;
            case InternalFormat::DepthComponent24: return GL_DEPTH_COMPONENT24;
            case InternalFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
            case InternalFormat::R32UI: return GL_R32UI;
            default: return GL_RGBA8;
        }
    }

    static GLenum ToGL(TextureWrap w) {
        switch (w) {
            case TextureWrap::Repeat: return GL_REPEAT;
            case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
            case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
            case TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER;
            default: return GL_REPEAT;
        }
    }

    static GLenum ToGL(TextureFilter f) {
        switch (f) {
            case TextureFilter::Nearest: return GL_NEAREST;
            case TextureFilter::Linear: return GL_LINEAR;
            case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
            case TextureFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
            case TextureFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
            case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
            default: return GL_LINEAR;
        }
    }

    static GLenum ToGL(CullMode m) {
        switch (m) {
            case CullMode::None: return 0;
            case CullMode::Front: return GL_FRONT;
            case CullMode::Back: return GL_BACK;
            case CullMode::FrontAndBack: return GL_FRONT_AND_BACK;
            default: return GL_BACK;
        }
    }

    static GLbitfield ToGL(BufferBit b) {
        GLbitfield flags = 0;
        if ((b & BufferBit::Color)) flags |= GL_COLOR_BUFFER_BIT;
        if ((b & BufferBit::Depth)) flags |= GL_DEPTH_BUFFER_BIT;
        if ((b & BufferBit::Stencil)) flags |= GL_STENCIL_BUFFER_BIT;
        return flags;
    }

    static GLenum ToGL(TextureParameter p) {
        switch (p) {
            case TextureParameter::MinFilter: return GL_TEXTURE_MIN_FILTER;
            case TextureParameter::MagFilter: return GL_TEXTURE_MAG_FILTER;
            case TextureParameter::WrapS: return GL_TEXTURE_WRAP_S;
            case TextureParameter::WrapT: return GL_TEXTURE_WRAP_T;
            case TextureParameter::WrapR: return GL_TEXTURE_WRAP_R;
            case TextureParameter::BorderColor: return GL_TEXTURE_BORDER_COLOR;
            default: return GL_TEXTURE_MIN_FILTER;
        }
    }

    static GLenum ToGL(CompareFunc f) {
        switch (f) {
            case CompareFunc::Never: return GL_NEVER;
            case CompareFunc::Less: return GL_LESS;
            case CompareFunc::Equal: return GL_EQUAL;
            case CompareFunc::Lequal: return GL_LEQUAL;
            case CompareFunc::Greater: return GL_GREATER;
            case CompareFunc::NotEqual: return GL_NOTEQUAL;
            case CompareFunc::Gequal: return GL_GEQUAL;
            case CompareFunc::Always: return GL_ALWAYS;
            default: return GL_LESS;
        }
    }

    static GLenum ToGL(StencilOp op) {
        switch (op) {
            case StencilOp::Keep: return GL_KEEP;
            case StencilOp::Zero: return GL_ZERO;
            case StencilOp::Replace: return GL_REPLACE;
            case StencilOp::Incr: return GL_INCR;
            case StencilOp::IncrWrap: return GL_INCR_WRAP;
            case StencilOp::Decr: return GL_DECR;
            case StencilOp::DecrWrap: return GL_DECR_WRAP;
            case StencilOp::Invert: return GL_INVERT;
            default: return GL_KEEP;
        }
    }

    static GLenum ToGL(TextureUnit u) {

        return GL_TEXTURE0 + static_cast<int>(u);
    }

    static GLenum ToGL(PolygonMode m) {
        switch (m) {
            case PolygonMode::Point: return GL_POINT;
            case PolygonMode::Line: return GL_LINE;
            case PolygonMode::Fill: return GL_FILL;
            default: return GL_FILL;
        }
    }

    static GLenum ToGL(PixelStoreParam p) {
        switch (p) {
            case PixelStoreParam::UnpackAlignment: return GL_UNPACK_ALIGNMENT;
            case PixelStoreParam::PackAlignment: return GL_PACK_ALIGNMENT;
            default: return GL_UNPACK_ALIGNMENT;
        }
    }

    static GLenum ToGL(ServerCapability c) {
        switch (c) {
            case ServerCapability::Blend: return GL_BLEND;
            case ServerCapability::CullFace: return GL_CULL_FACE;
            case ServerCapability::DepthTest: return GL_DEPTH_TEST;
            case ServerCapability::StencilTest: return GL_STENCIL_TEST;
            case ServerCapability::ScissorTest: return GL_SCISSOR_TEST;
            case ServerCapability::Multisample: return GL_MULTISAMPLE;

            default: return 0;
        }
    }

    static GLenum ToGL(BlendFactor f) {
        switch (f) {
            case BlendFactor::Zero: return GL_ZERO;
            case BlendFactor::One: return GL_ONE;
            case BlendFactor::SrcColor: return GL_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
            case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha: return GL_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
            case BlendFactor::DstColor: return GL_DST_COLOR;
            case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
            case BlendFactor::ConstantColor: return GL_CONSTANT_COLOR;
            case BlendFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
            case BlendFactor::ConstantAlpha: return GL_CONSTANT_ALPHA;
            case BlendFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
            default: return GL_ONE;
        }
    }

    static GLenum ToGL(ShaderType t) {
        switch (t) {
            case ShaderType::Vertex: return GL_VERTEX_SHADER;
            case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
            case ShaderType::Geometry: return GL_GEOMETRY_SHADER;
            case ShaderType::Compute: return GL_COMPUTE_SHADER;
            default: return GL_VERTEX_SHADER;
        }
    }

    static GLenum ToGL(FrontFace f) {
        switch (f) {
            case FrontFace::CW: return GL_CW;
            case FrontFace::CCW: return GL_CCW;
            default: return GL_CCW;
        }
    }

    static GLenum ToGL(BlendEquation e) {
        switch (e) {
            case BlendEquation::Add: return GL_FUNC_ADD;
            case BlendEquation::Subtract: return GL_FUNC_SUBTRACT;
            case BlendEquation::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
            case BlendEquation::Min: return GL_MIN;
            case BlendEquation::Max: return GL_MAX;
            default: return GL_FUNC_ADD;
        }
    }

    static GLenum ToGL(FramebufferTarget t) {
        switch (t) {
            case FramebufferTarget::Framebuffer: return GL_FRAMEBUFFER;
            case FramebufferTarget::ReadFramebuffer: return GL_READ_FRAMEBUFFER;
            case FramebufferTarget::DrawFramebuffer: return GL_DRAW_FRAMEBUFFER;
            default: return GL_FRAMEBUFFER;
        }
    }

    static GLenum ToGL(FramebufferAttachment a) {
        switch (a) {
            case FramebufferAttachment::None: return GL_NONE;
            case FramebufferAttachment::Color0: return GL_COLOR_ATTACHMENT0;
            case FramebufferAttachment::Color1: return GL_COLOR_ATTACHMENT1;
            case FramebufferAttachment::Color2: return GL_COLOR_ATTACHMENT2;
            case FramebufferAttachment::Color3: return GL_COLOR_ATTACHMENT3;
            case FramebufferAttachment::Depth: return GL_DEPTH_ATTACHMENT;
            case FramebufferAttachment::Stencil: return GL_STENCIL_ATTACHMENT;
            case FramebufferAttachment::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
            default: return GL_COLOR_ATTACHMENT0;
        }
    }

    static GLbitfield ToGL(MemoryBarrierBit b) {
        switch (b) {
            case MemoryBarrierBit::ShaderImageAccess: return GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
            case MemoryBarrierBit::VertexAttribArray: return GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
            case MemoryBarrierBit::ElementArray: return GL_ELEMENT_ARRAY_BARRIER_BIT;
            case MemoryBarrierBit::Uniform: return GL_UNIFORM_BARRIER_BIT;
            case MemoryBarrierBit::TextureFetch: return GL_TEXTURE_FETCH_BARRIER_BIT;
            case MemoryBarrierBit::BufferUpdate: return GL_BUFFER_UPDATE_BARRIER_BIT;
            case MemoryBarrierBit::FrameBuffer: return GL_FRAMEBUFFER_BARRIER_BIT;
            case MemoryBarrierBit::All: return GL_ALL_BARRIER_BITS;
            default: return GL_ALL_BARRIER_BITS;
        }
    }

    static GLenum ToGL(QueryType t) {
        switch (t) {
            case QueryType::SamplesPassed: return GL_SAMPLES_PASSED;
            case QueryType::AnySamplesPassed: return GL_ANY_SAMPLES_PASSED;
            case QueryType::AnySamplesPassedConservative: return GL_ANY_SAMPLES_PASSED_CONSERVATIVE;
            case QueryType::TimeElapsed: return GL_TIME_ELAPSED;
            case QueryType::Timestamp: return GL_TIMESTAMP;
            default: return GL_SAMPLES_PASSED;
        }
    }
};