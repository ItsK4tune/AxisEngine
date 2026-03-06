#pragma once

namespace Graphics {

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

    enum class TextureParameter {
        MinFilter,
        MagFilter,
        WrapS,
        WrapT,
        WrapR,
        BorderColor
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
}
