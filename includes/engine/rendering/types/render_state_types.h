#pragma once

namespace Graphics {

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
}
