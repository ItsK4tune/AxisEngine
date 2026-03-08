#pragma once

#include <cstddef>
#include <render/type/graphics_types.h>

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
    virtual void TexParameterfv(TextureType target, TextureParameter pname, const float *params) = 0;
    virtual void GenerateMipmap(TextureType target) = 0;

    virtual void TexImage2D(TextureType target, int level, InternalFormat internalFormat,
                            int width, int height, int border,
                            TextureFormat format, DataType type, const void *data) = 0;
    virtual void TexSubImage2D(TextureType target, int level, int xoffset, int yoffset,
                               int width, int height, TextureFormat format,
                               DataType type, const void *data) = 0;

    virtual void ActiveTexture(TextureUnit unit) = 0;
    virtual void PixelStorei(PixelStoreParam pname, int param) = 0;

    virtual const char *GetBackendName() const = 0;
};