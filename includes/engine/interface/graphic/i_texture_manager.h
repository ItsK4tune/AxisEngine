#pragma once

#include <cstddef>
#include <interface/graphic/graphics_types.h>

class ITextureManager
{
public:
    virtual ~ITextureManager() = default;

    virtual unsigned int CreateTexture() = 0;
    virtual unsigned int GenTexture() = 0;
    virtual void BindTexture(Graphics::TextureType target, unsigned int texture) = 0;
    virtual void DeleteTexture(unsigned int texture) = 0;
    virtual void DeleteTextures(int n, const unsigned int* textures) = 0;

    virtual void TexParameteri(Graphics::TextureType target, Graphics::TextureParameter pname, int param) = 0;
    virtual void TexParameterfv(Graphics::TextureType target, Graphics::TextureParameter pname, const float *params) = 0;
    virtual void GenerateMipmap(Graphics::TextureType target) = 0;

    virtual void TexImage2D(Graphics::TextureType target, int level, Graphics::InternalFormat internalFormat,
                            int width, int height, int border,
                            Graphics::TextureFormat format, Graphics::DataType type, const void *data) = 0;
    virtual void TexSubImage2D(Graphics::TextureType target, int level, int xoffset, int yoffset,
                               int width, int height, Graphics::TextureFormat format,
                               Graphics::DataType type, const void *data) = 0;

    virtual void ActiveTexture(Graphics::TextureUnit unit) = 0;
    virtual void PixelStorei(Graphics::PixelStoreParam pname, int param) = 0;

    virtual const char *GetBackendName() const = 0;
};
