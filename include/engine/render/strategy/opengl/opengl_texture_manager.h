#include <render/interface/i_texture_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>

class OpenGLTextureManager : public ITextureManager
{
public:
private:
public:
    unsigned int CreateTexture() override
    {
        unsigned int Texture;
        glGenTextures(1, &Texture);
        return Texture;
    }

    unsigned int GenTexture() override
    {
        return CreateTexture();
    }

    void BindTexture(TextureType target, unsigned int Texture) override
    {
        glBindTexture(GLTranslator::ToGL(target), Texture);
    }

    void DeleteTexture(unsigned int Texture) override
    {
        glDeleteTextures(1, &Texture);
    }
    void DeleteTextures(int n, const unsigned int* textures) override
    {
        glDeleteTextures(n, textures);
    }

    void TexParameteri(TextureType target, TextureParameter pname, int param) override
    {
        GLint glParam = param;
        if (pname == TextureParameter::WrapS || pname == TextureParameter::WrapT || pname == TextureParameter::WrapR)
        {
            glParam = GLTranslator::ToGL(static_cast<TextureWrap>(param));
        }
        else if (pname == TextureParameter::MinFilter || pname == TextureParameter::MagFilter)
        {
            glParam = GLTranslator::ToGL(static_cast<TextureFilter>(param));
        }
        glTexParameteri(GLTranslator::ToGL(target), GLTranslator::ToGL(pname), glParam);
    }

    void TexParameterf(TextureType target, TextureParameter pname, float param) override
    {
        glTexParameterf(GLTranslator::ToGL(target), GLTranslator::ToGL(pname), param);
    }

    void TexParameterfv(TextureType target, TextureParameter pname, const float* params) override
    {
        glTexParameterfv(GLTranslator::ToGL(target), GLTranslator::ToGL(pname), params);
    }

    void TexParameteriv(TextureType target, TextureParameter pname, const int* params) override
    {
        if (pname == TextureParameter::SwizzleRGBA)
        {
            GLint glParams[4] = {
                static_cast<GLint>(GLTranslator::ToGL(static_cast<TextureSwizzle>(params[0]))),
                static_cast<GLint>(GLTranslator::ToGL(static_cast<TextureSwizzle>(params[1]))),
                static_cast<GLint>(GLTranslator::ToGL(static_cast<TextureSwizzle>(params[2]))),
                static_cast<GLint>(GLTranslator::ToGL(static_cast<TextureSwizzle>(params[3]))),
            };
            glTexParameteriv(GLTranslator::ToGL(target), GLTranslator::ToGL(pname), glParams);
            return;
        }

        glTexParameteriv(GLTranslator::ToGL(target), GLTranslator::ToGL(pname), params);
    }

    void GenerateMipmap(TextureType target) override
    {
        glGenerateMipmap(GLTranslator::ToGL(target));
    }

    void TexImage1D(TextureType target, int level, InternalFormat internalFormat, int width, int border,
                    TextureFormat format, DataType type, const void* data) override
    {
        glTexImage1D(GLTranslator::ToGL(target), level, GLTranslator::ToGL(internalFormat), width, border,
                     GLTranslator::ToGL(format), GLTranslator::ToGL(type), data);
    }

    void TexImage2D(TextureType target, int level, InternalFormat internalFormat, int width, int height, int border,
                    TextureFormat format, DataType type, const void* data) override
    {
        glTexImage2D(GLTranslator::ToGL(target), level, GLTranslator::ToGL(internalFormat), width, height, border,
                     GLTranslator::ToGL(format), GLTranslator::ToGL(type), data);
    }

    void TexImage3D(TextureType target, int level, InternalFormat internalFormat, int width, int height, int depth,
                    int border, TextureFormat format, DataType type, const void* data) override
    {
        glTexImage3D(GLTranslator::ToGL(target), level, GLTranslator::ToGL(internalFormat), width, height, depth,
                     border, GLTranslator::ToGL(format), GLTranslator::ToGL(type), data);
    }

    void TexSubImage2D(TextureType target, int level, int xoffset, int yoffset, int width, int height,
                       TextureFormat format, DataType type, const void* data) override
    {
        glTexSubImage2D(GLTranslator::ToGL(target), level, xoffset, yoffset, width, height, GLTranslator::ToGL(format),
                        GLTranslator::ToGL(type), data);
    }

    void ActiveTexture(TextureUnit unit) override
    {
        glActiveTexture(GLTranslator::ToGL(unit));
    }

    void PixelStorei(PixelStoreParam pname, int param) override
    {
        glPixelStorei(GLTranslator::ToGL(pname), param);
    }

    void* GetNativeTextureHandle(unsigned int texture) const override
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(texture));
    }

    const char* GetBackendName() const override
    {
        return "OpenGL";
    }
};
