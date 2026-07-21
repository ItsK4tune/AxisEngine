#pragma once

#include <render/type/graphics_types.h>
#include <cstddef>

class ITextureManager
{
public:
    virtual ~ITextureManager() = default;

    virtual unsigned int CreateTexture() = 0;
    virtual unsigned int GenTexture() = 0;
    virtual void BindTexture(TextureType target, unsigned int Texture) = 0;
    virtual void DeleteTexture(unsigned int Texture) = 0;
    virtual void DeleteTextures(int n, const unsigned int* textures) = 0;

    virtual void TexParameteri(TextureType target, TextureParameter pname, int param) = 0;
    virtual void TexParameterf(TextureType target, TextureParameter pname, float param) = 0;
    virtual void TexParameterfv(TextureType target, TextureParameter pname, const float* params) = 0;
    virtual void GenerateMipmap(TextureType target) = 0;

    virtual void TexImage1D(TextureType target, int level, InternalFormat internalFormat, int width, int border,
                            TextureFormat format, DataType type, const void* data) = 0;
    virtual void TexImage2D(TextureType target, int level, InternalFormat internalFormat, int width, int height,
                            int border, TextureFormat format, DataType type, const void* data) = 0;
    virtual bool CompressedTexImage2D(TextureType, int, InternalFormat, int, int, size_t, const void*)
    {
        return false;
    }
    virtual bool TexImage2DMultisample(TextureType, int, InternalFormat, int, int, bool)
    {
        return false;
    }
    virtual void TexImage3D(TextureType target, int level, InternalFormat internalFormat, int width, int height,
                            int depth, int border, TextureFormat format, DataType type, const void* data) = 0;
    virtual void TexSubImage2D(TextureType target, int level, int xoffset, int yoffset, int width, int height,
                               TextureFormat format, DataType type, const void* data) = 0;

    virtual void ActiveTexture(TextureUnit unit) = 0;
    virtual void PixelStorei(PixelStoreParam pname, int param) = 0;
    virtual bool SetTextureSwizzle(TextureType, unsigned int, TextureSwizzle)
    {
        return false;
    }

    virtual const char* GetBackendName() const = 0;
};
