#pragma once

#include <glad/glad.h>
#include <interface/graphic/graphics_types.h>
#include <utils/logger.h>

class GLTranslator
{
public:
    static GLenum ToGL(Graphics::Primitive p) {
        switch (p) {
            case Graphics::Primitive::Points: return GL_POINTS;
            case Graphics::Primitive::Lines: return GL_LINES;
            case Graphics::Primitive::LineLoop: return GL_LINE_LOOP;
            case Graphics::Primitive::LineStrip: return GL_LINE_STRIP;
            case Graphics::Primitive::Triangles: return GL_TRIANGLES;
            case Graphics::Primitive::TriangleStrip: return GL_TRIANGLE_STRIP;
            case Graphics::Primitive::TriangleFan: return GL_TRIANGLE_FAN;
            default: return GL_TRIANGLES;
        }
    }

    static GLenum ToGL(Graphics::DataType t) {
        switch (t) {
            case Graphics::DataType::Byte: return GL_BYTE;
            case Graphics::DataType::UnsignedByte: return GL_UNSIGNED_BYTE;
            case Graphics::DataType::Short: return GL_SHORT;
            case Graphics::DataType::UnsignedShort: return GL_UNSIGNED_SHORT;
            case Graphics::DataType::Int: return GL_INT;
            case Graphics::DataType::UnsignedInt: return GL_UNSIGNED_INT;
            case Graphics::DataType::Float: return GL_FLOAT;
            case Graphics::DataType::Double: return GL_DOUBLE;
            default: return GL_FLOAT;
        }
    }

    static GLenum ToGL(Graphics::BufferType t) {
        switch (t) {
            case Graphics::BufferType::ArrayBuffer: return GL_ARRAY_BUFFER;
            case Graphics::BufferType::ElementArrayBuffer: return GL_ELEMENT_ARRAY_BUFFER;
            case Graphics::BufferType::UniformBuffer: return GL_UNIFORM_BUFFER;
            case Graphics::BufferType::ShaderStorageBuffer: return GL_SHADER_STORAGE_BUFFER;
            default: return GL_ARRAY_BUFFER;
        }
    }

    static GLenum ToGL(Graphics::BufferUsage u) {
        switch (u) {
            case Graphics::BufferUsage::StreamDraw: return GL_STREAM_DRAW;
            case Graphics::BufferUsage::StreamRead: return GL_STREAM_READ;
            case Graphics::BufferUsage::StreamCopy: return GL_STREAM_COPY;
            case Graphics::BufferUsage::StaticDraw: return GL_STATIC_DRAW;
            case Graphics::BufferUsage::StaticRead: return GL_STATIC_READ;
            case Graphics::BufferUsage::StaticCopy: return GL_STATIC_COPY;
            case Graphics::BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
            case Graphics::BufferUsage::DynamicRead: return GL_DYNAMIC_READ;
            case Graphics::BufferUsage::DynamicCopy: return GL_DYNAMIC_COPY;
            default: return GL_STATIC_DRAW;
        }
    }

    static GLenum ToGL(Graphics::TextureType t) {
        switch (t) {
            case Graphics::TextureType::Texture2D: return GL_TEXTURE_2D;
            case Graphics::TextureType::TextureCubeMap: return GL_TEXTURE_CUBE_MAP;
            case Graphics::TextureType::Texture3D: return GL_TEXTURE_3D;
            case Graphics::TextureType::CubeMapPositiveX: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            case Graphics::TextureType::CubeMapNegativeX: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
            case Graphics::TextureType::CubeMapPositiveY: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
            case Graphics::TextureType::CubeMapNegativeY: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
            case Graphics::TextureType::CubeMapPositiveZ: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
            case Graphics::TextureType::CubeMapNegativeZ: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
            default: return GL_TEXTURE_2D;
        }
    }

    static GLenum ToGL(Graphics::TextureFormat f) {
        switch (f) {
            case Graphics::TextureFormat::Red: return GL_RED;
            case Graphics::TextureFormat::RG: return GL_RG;
            case Graphics::TextureFormat::RGB: return GL_RGB;
            case Graphics::TextureFormat::RGBA: return GL_RGBA;
            case Graphics::TextureFormat::DepthComponent: return GL_DEPTH_COMPONENT;
            case Graphics::TextureFormat::DepthStencil: return GL_DEPTH_STENCIL;
            default: return GL_RGBA;
        }
    }

    static GLenum ToGL(Graphics::InternalFormat f) {
        switch (f) {
            case Graphics::InternalFormat::R8: return GL_R8;
            case Graphics::InternalFormat::RGB8: return GL_RGB8;
            case Graphics::InternalFormat::RGBA8: return GL_RGBA8;
            case Graphics::InternalFormat::RGBA16F: return GL_RGBA16F;
            case Graphics::InternalFormat::DepthComponent24: return GL_DEPTH_COMPONENT24;
            case Graphics::InternalFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
            default: return GL_RGBA8;
        }
    }

    static GLenum ToGL(Graphics::TextureWrap w) {
        switch (w) {
            case Graphics::TextureWrap::Repeat: return GL_REPEAT;
            case Graphics::TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
            case Graphics::TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
            case Graphics::TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER;
            default: return GL_REPEAT;
        }
    }

    static GLenum ToGL(Graphics::TextureFilter f) {
        switch (f) {
            case Graphics::TextureFilter::Nearest: return GL_NEAREST;
            case Graphics::TextureFilter::Linear: return GL_LINEAR;
            case Graphics::TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
            case Graphics::TextureFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
            case Graphics::TextureFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
            case Graphics::TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
            default: return GL_LINEAR;
        }
    }

    static GLenum ToGL(Graphics::CullMode m) {
        switch (m) {
            case Graphics::CullMode::None: return 0;
            case Graphics::CullMode::Front: return GL_FRONT;
            case Graphics::CullMode::Back: return GL_BACK;
            case Graphics::CullMode::FrontAndBack: return GL_FRONT_AND_BACK;
            default: return GL_BACK;
        }
    }

    static GLbitfield ToGL(Graphics::BufferBit b) {
        GLbitfield flags = 0;
        if ((b & Graphics::BufferBit::Color)) flags |= GL_COLOR_BUFFER_BIT;
        if ((b & Graphics::BufferBit::Depth)) flags |= GL_DEPTH_BUFFER_BIT;
        if ((b & Graphics::BufferBit::Stencil)) flags |= GL_STENCIL_BUFFER_BIT;
        return flags;
    }

    static GLenum ToGL(Graphics::TextureParameter p) {
        switch (p) {
            case Graphics::TextureParameter::MinFilter: return GL_TEXTURE_MIN_FILTER;
            case Graphics::TextureParameter::MagFilter: return GL_TEXTURE_MAG_FILTER;
            case Graphics::TextureParameter::WrapS: return GL_TEXTURE_WRAP_S;
            case Graphics::TextureParameter::WrapT: return GL_TEXTURE_WRAP_T;
            case Graphics::TextureParameter::WrapR: return GL_TEXTURE_WRAP_R;
            case Graphics::TextureParameter::BorderColor: return GL_TEXTURE_BORDER_COLOR;
            default: return GL_TEXTURE_MIN_FILTER;
        }
    }

    static GLenum ToGL(Graphics::CompareFunc f) {
        switch (f) {
            case Graphics::CompareFunc::Never: return GL_NEVER;
            case Graphics::CompareFunc::Less: return GL_LESS;
            case Graphics::CompareFunc::Equal: return GL_EQUAL;
            case Graphics::CompareFunc::Lequal: return GL_LEQUAL;
            case Graphics::CompareFunc::Greater: return GL_GREATER;
            case Graphics::CompareFunc::NotEqual: return GL_NOTEQUAL;
            case Graphics::CompareFunc::Gequal: return GL_GEQUAL;
            case Graphics::CompareFunc::Always: return GL_ALWAYS;
            default: return GL_LESS;
        }
    }

    static GLenum ToGL(Graphics::StencilOp op) {
        switch (op) {
            case Graphics::StencilOp::Keep: return GL_KEEP;
            case Graphics::StencilOp::Zero: return GL_ZERO;
            case Graphics::StencilOp::Replace: return GL_REPLACE;
            case Graphics::StencilOp::Incr: return GL_INCR;
            case Graphics::StencilOp::IncrWrap: return GL_INCR_WRAP;
            case Graphics::StencilOp::Decr: return GL_DECR;
            case Graphics::StencilOp::DecrWrap: return GL_DECR_WRAP;
            case Graphics::StencilOp::Invert: return GL_INVERT;
            default: return GL_KEEP;
        }
    }

    static GLenum ToGL(Graphics::TextureUnit u) {

        return GL_TEXTURE0 + static_cast<int>(u);
    }

    static GLenum ToGL(Graphics::PolygonMode m) {
        switch (m) {
            case Graphics::PolygonMode::Point: return GL_POINT;
            case Graphics::PolygonMode::Line: return GL_LINE;
            case Graphics::PolygonMode::Fill: return GL_FILL;
            default: return GL_FILL;
        }
    }

    static GLenum ToGL(Graphics::PixelStoreParam p) {
        switch (p) {
            case Graphics::PixelStoreParam::UnpackAlignment: return GL_UNPACK_ALIGNMENT;
            case Graphics::PixelStoreParam::PackAlignment: return GL_PACK_ALIGNMENT;
            default: return GL_UNPACK_ALIGNMENT;
        }
    }

    static GLenum ToGL(Graphics::ServerCapability c) {
        switch (c) {
            case Graphics::ServerCapability::Blend: return GL_BLEND;
            case Graphics::ServerCapability::CullFace: return GL_CULL_FACE;
            case Graphics::ServerCapability::DepthTest: return GL_DEPTH_TEST;
            case Graphics::ServerCapability::StencilTest: return GL_STENCIL_TEST;
            case Graphics::ServerCapability::ScissorTest: return GL_SCISSOR_TEST;
            case Graphics::ServerCapability::Multisample: return GL_MULTISAMPLE;

            default: return 0;
        }
    }

    static GLenum ToGL(Graphics::BlendFactor f) {
        switch (f) {
            case Graphics::BlendFactor::Zero: return GL_ZERO;
            case Graphics::BlendFactor::One: return GL_ONE;
            case Graphics::BlendFactor::SrcColor: return GL_SRC_COLOR;
            case Graphics::BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
            case Graphics::BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
            case Graphics::BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
            case Graphics::BlendFactor::DstAlpha: return GL_DST_ALPHA;
            case Graphics::BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
            case Graphics::BlendFactor::DstColor: return GL_DST_COLOR;
            case Graphics::BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
            case Graphics::BlendFactor::ConstantColor: return GL_CONSTANT_COLOR;
            case Graphics::BlendFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
            case Graphics::BlendFactor::ConstantAlpha: return GL_CONSTANT_ALPHA;
            case Graphics::BlendFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
            default: return GL_ONE;
        }
    }

    static GLenum ToGL(Graphics::ShaderType t) {
        switch (t) {
            case Graphics::ShaderType::Vertex: return GL_VERTEX_SHADER;
            case Graphics::ShaderType::Fragment: return GL_FRAGMENT_SHADER;
            case Graphics::ShaderType::Geometry: return GL_GEOMETRY_SHADER;
            case Graphics::ShaderType::Compute: return GL_COMPUTE_SHADER;
            default: return GL_VERTEX_SHADER;
        }
    }

    static GLenum ToGL(Graphics::FrontFace f) {
        switch (f) {
            case Graphics::FrontFace::CW: return GL_CW;
            case Graphics::FrontFace::CCW: return GL_CCW;
            default: return GL_CCW;
        }
    }

    static GLenum ToGL(Graphics::BlendEquation e) {
        switch (e) {
            case Graphics::BlendEquation::Add: return GL_FUNC_ADD;
            case Graphics::BlendEquation::Subtract: return GL_FUNC_SUBTRACT;
            case Graphics::BlendEquation::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
            case Graphics::BlendEquation::Min: return GL_MIN;
            case Graphics::BlendEquation::Max: return GL_MAX;
            default: return GL_FUNC_ADD;
        }
    }

    static GLenum ToGL(Graphics::FramebufferTarget t) {
        switch (t) {
            case Graphics::FramebufferTarget::Framebuffer: return GL_FRAMEBUFFER;
            case Graphics::FramebufferTarget::ReadFramebuffer: return GL_READ_FRAMEBUFFER;
            case Graphics::FramebufferTarget::DrawFramebuffer: return GL_DRAW_FRAMEBUFFER;
            default: return GL_FRAMEBUFFER;
        }
    }

    static GLenum ToGL(Graphics::FramebufferAttachment a) {
        switch (a) {
            case Graphics::FramebufferAttachment::None: return GL_NONE;
            case Graphics::FramebufferAttachment::Color0: return GL_COLOR_ATTACHMENT0;
            case Graphics::FramebufferAttachment::Color1: return GL_COLOR_ATTACHMENT1;
            case Graphics::FramebufferAttachment::Color2: return GL_COLOR_ATTACHMENT2;
            case Graphics::FramebufferAttachment::Color3: return GL_COLOR_ATTACHMENT3;
            case Graphics::FramebufferAttachment::Depth: return GL_DEPTH_ATTACHMENT;
            case Graphics::FramebufferAttachment::Stencil: return GL_STENCIL_ATTACHMENT;
            case Graphics::FramebufferAttachment::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
            default: return GL_COLOR_ATTACHMENT0;
        }
    }

    static GLbitfield ToGL(Graphics::MemoryBarrierBit b) {
        switch (b) {
            case Graphics::MemoryBarrierBit::ShaderImageAccess: return GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
            case Graphics::MemoryBarrierBit::VertexAttribArray: return GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
            case Graphics::MemoryBarrierBit::ElementArray: return GL_ELEMENT_ARRAY_BARRIER_BIT;
            case Graphics::MemoryBarrierBit::Uniform: return GL_UNIFORM_BARRIER_BIT;
            case Graphics::MemoryBarrierBit::TextureFetch: return GL_TEXTURE_FETCH_BARRIER_BIT;
            case Graphics::MemoryBarrierBit::BufferUpdate: return GL_BUFFER_UPDATE_BARRIER_BIT;
            case Graphics::MemoryBarrierBit::FrameBuffer: return GL_FRAMEBUFFER_BARRIER_BIT;
            case Graphics::MemoryBarrierBit::All: return GL_ALL_BARRIER_BITS;
            default: return GL_ALL_BARRIER_BITS;
        }
    }

    static GLenum ToGL(Graphics::QueryType t) {
        switch (t) {
            case Graphics::QueryType::SamplesPassed: return GL_SAMPLES_PASSED;
            case Graphics::QueryType::AnySamplesPassed: return GL_ANY_SAMPLES_PASSED;
            case Graphics::QueryType::AnySamplesPassedConservative: return GL_ANY_SAMPLES_PASSED_CONSERVATIVE;
            case Graphics::QueryType::TimeElapsed: return GL_TIME_ELAPSED;
            case Graphics::QueryType::Timestamp: return GL_TIMESTAMP;
            default: return GL_SAMPLES_PASSED;
        }
    }
};
