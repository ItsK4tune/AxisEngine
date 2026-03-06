#pragma once

namespace Graphics {

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
}
