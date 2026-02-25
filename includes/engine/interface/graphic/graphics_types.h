#pragma once

#include <cstdint>

namespace Graphics {

    enum class Primitive {
        Points,
        Lines,
        LineLoop,
        LineStrip,
        Triangles,
        TriangleStrip,
        TriangleFan
    };

    enum class DataType {
        Byte,
        UnsignedByte,
        Short,
        UnsignedShort,
        Int,
        UnsignedInt,
        Float,
        Double
    };

    enum class BufferType {
        ArrayBuffer,
        ElementArrayBuffer,
        UniformBuffer,
        ShaderStorageBuffer
    };

    enum class BufferUsage {
        StreamDraw,
        StreamRead,
        StreamCopy,
        StaticDraw,
        StaticRead,
        StaticCopy,
        DynamicDraw,
        DynamicRead,
        DynamicCopy
    };

    enum class TextureType {
        Texture2D,
        TextureCubeMap,
        Texture3D,
        CubeMapPositiveX,
        CubeMapNegativeX,
        CubeMapPositiveY,
        CubeMapNegativeY,
        CubeMapPositiveZ,
        CubeMapNegativeZ
    };

    enum class TextureFormat {
        Red,
        RG,
        RGB,
        RGBA,
        DepthComponent,
        DepthStencil
    };

    enum class InternalFormat {
        R8,
        RGB8,
        RGBA8,
        RGBA16F,
        DepthComponent24,
        Depth24Stencil8
    };

    enum class TextureWrap {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class TextureFilter {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };

    enum class CullMode {
        None,
        Front,
        Back,
        FrontAndBack
    };

    enum class BufferBit {
        Color = 1,
        Depth = 2,
        Stencil = 4
    };

    inline BufferBit operator|(BufferBit a, BufferBit b) {
        return static_cast<BufferBit>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline bool operator&(BufferBit a, BufferBit b) {
        return (static_cast<int>(a) & static_cast<int>(b)) != 0;
    }

    enum class TextureParameter {
        MinFilter,
        MagFilter,
        WrapS,
        WrapT,
        WrapR,
        BorderColor
    };

    enum class CompareFunc {
        Never,
        Less,
        Equal,
        Lequal,
        Greater,
        NotEqual,
        Gequal,
        Always
    };

    enum class StencilOp {
        Keep,
        Zero,
        Replace,
        Incr,
        IncrWrap,
        Decr,
        DecrWrap,
        Invert
    };

    enum class TextureUnit {
        Texture0,
        Texture1,
        Texture2,
        Texture3,
        Texture4,
        Texture5,
        Texture6,
        Texture7
    };

    enum class PolygonMode {
        Point,
        Line,
        Fill
    };

    enum class PixelStoreParam {
        UnpackAlignment,
        PackAlignment
    };

    enum class ServerCapability {
        Blend,
        CullFace,
        DepthTest,
        StencilTest,
        ScissorTest,
        Multisample
    };

    enum class BlendFactor {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        DstColor,
        OneMinusDstColor,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha
    };

    enum class FrontFace {
        CW,
        CCW
    };

    enum class BlendEquation {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class ShaderType {
        Vertex,
        Fragment,
        Geometry,
        Compute
    };

    enum class FramebufferTarget {
        Framebuffer,
        ReadFramebuffer,
        DrawFramebuffer
    };

    enum class FramebufferAttachment {
        None,
        Color0,
        Color1,
        Color2,
        Color3,
        Depth,
        Stencil,
        DepthStencil
    };

    enum class MemoryBarrierBit {
        ShaderImageAccess,
        VertexAttribArray,
        ElementArray,
        Uniform,
        TextureFetch,
        BufferUpdate,
        FrameBuffer,
        All
    };
    enum class FramebufferStatus {
        Complete,
        Undefined,
        IncompleteAttachment,
        IncompleteMissingAttachment,
        IncompleteDrawBuffer,
        IncompleteReadBuffer,
        Unsupported,
        IncompleteMultisample,
        IncompleteLayerTargets,
        Unknown
    };
    
    enum class QueryType {
        SamplesPassed,
        AnySamplesPassed,
        AnySamplesPassedConservative,
        TimeElapsed,
        Timestamp
    };

    struct GpuHandle {
        uint32_t id = 0;
        bool IsValid() const { return id != 0; }
        void Reset() { id = 0; }
        bool operator==(const GpuHandle& other) const { return id == other.id; }
        bool operator!=(const GpuHandle& other) const { return id != other.id; }
    };
}
