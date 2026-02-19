#include <interface/graphic/i_texture_manager.h>
#include <graphic/backends/opengl_translator.h>
#include <glad/glad.h>

class OpenGLTextureManager : public ITextureManager
{
public:
private:
    


public:


    unsigned int CreateTexture() override
    {
        unsigned int texture;
        glGenTextures(1, &texture);
        return texture;
    }

    unsigned int GenTexture() override { return CreateTexture(); }

    void BindTexture(Graphics::TextureType target, unsigned int texture) override { glBindTexture(GLTranslator::ToGL(target), texture); }

    void DeleteTexture(unsigned int texture) override { glDeleteTextures(1, &texture); }
    void DeleteTextures(int n, const unsigned int* textures) override { glDeleteTextures(n, textures); }

    void TexParameteri(Graphics::TextureType target, Graphics::TextureParameter pname, int param) override
    {
        GLint glParam = param;
        if (pname == Graphics::TextureParameter::WrapS || pname == Graphics::TextureParameter::WrapT || pname == Graphics::TextureParameter::WrapR) {
            glParam = GLTranslator::ToGL(static_cast<Graphics::TextureWrap>(param));
        } else if (pname == Graphics::TextureParameter::MinFilter || pname == Graphics::TextureParameter::MagFilter) {
            glParam = GLTranslator::ToGL(static_cast<Graphics::TextureFilter>(param));
        }
        glTexParameteri(GLTranslator::ToGL(target), GLTranslator::ToGL(pname), glParam);
    }

    void TexParameterfv(Graphics::TextureType target, Graphics::TextureParameter pname, const float *params) override
    {
        glTexParameterfv(GLTranslator::ToGL(target), GLTranslator::ToGL(pname), params);
    }

    void GenerateMipmap(Graphics::TextureType target) override { glGenerateMipmap(GLTranslator::ToGL(target)); }

    void TexImage2D(Graphics::TextureType target, int level, Graphics::InternalFormat internalFormat,
                    int width, int height, int border,
                    Graphics::TextureFormat format, Graphics::DataType type, const void *data) override
    {
        glTexImage2D(GLTranslator::ToGL(target), level, GLTranslator::ToGL(internalFormat), width, height, border, GLTranslator::ToGL(format), GLTranslator::ToGL(type), data);
    }

    void TexSubImage2D(Graphics::TextureType target, int level, int xoffset, int yoffset,
                       int width, int height, Graphics::TextureFormat format,
                       Graphics::DataType type, const void *data) override
    {
        glTexSubImage2D(GLTranslator::ToGL(target), level, xoffset, yoffset, width, height, GLTranslator::ToGL(format), GLTranslator::ToGL(type), data);
    }

    void ActiveTexture(Graphics::TextureUnit unit) override {
        glActiveTexture(GLTranslator::ToGL(unit));
    }

    void PixelStorei(Graphics::PixelStoreParam pname, int param) override {
        glPixelStorei(GLTranslator::ToGL(pname), param);
    }

    const char *GetBackendName() const override { return "OpenGL"; }
};
