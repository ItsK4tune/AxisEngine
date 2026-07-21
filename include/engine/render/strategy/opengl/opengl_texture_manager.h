#include <render/interface/i_texture_manager.h>
#include <render/strategy/opengl/opengl_translator.h>
#include <glad/glad.h>
#include <core/logic/runtime_profiler.h>
#include <array>
#include <limits>

class OpenGLTextureManager : public ITextureManager
{
public:
    OpenGLTextureManager()
    {
        InvalidateCache();
    }

    void InvalidateCache()
    {
        m_ActiveUnit = InvalidBinding;
        for (auto& unit : m_BoundTextures)
            unit.fill(InvalidBinding);
    }

    void SetCacheEnabled(bool enabled)
    {
        m_CacheEnabled = enabled;
        InvalidateCache();
    }

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
        EnsureActiveUnit();
        const TextureType bindingTarget = NormalizeBindingTarget(target);
        auto& current = m_BoundTextures[m_ActiveUnit][static_cast<size_t>(bindingTarget)];
        if (!m_CacheEnabled || current != Texture)
        {
            glBindTexture(GLTranslator::ToGL(bindingTarget), Texture);
            current = Texture;
            RuntimeProfiler::Instance().AddStateChanges();
        }
    }

    void DeleteTexture(unsigned int Texture) override
    {
        glDeleteTextures(1, &Texture);
        for (auto& unit : m_BoundTextures)
            for (auto& bound : unit)
                if (bound == Texture)
                    bound = InvalidBinding;
    }
    void DeleteTextures(int n, const unsigned int* textures) override
    {
        glDeleteTextures(n, textures);
        InvalidateCache();
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

    bool CompressedTexImage2D(TextureType target, int level, InternalFormat internalFormat, int width, int height,
                              size_t dataSize, const void* data) override
    {
        if (!data || dataSize == 0)
            return false;
        glCompressedTexImage2D(GLTranslator::ToGL(target), level, GLTranslator::ToGL(internalFormat), width, height,
                               0, static_cast<GLsizei>(dataSize), data);
        RuntimeProfiler::Instance().AddUploadBytes(dataSize);
        return true;
    }

    bool TexImage2DMultisample(TextureType target, int samples, InternalFormat internalFormat, int width, int height,
                               bool fixedSampleLocations) override
    {
        if (target != TextureType::Texture2DMultisample || samples <= 1)
            return false;
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GLTranslator::ToGL(internalFormat), width, height,
                                fixedSampleLocations ? GL_TRUE : GL_FALSE);
        return true;
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
        const unsigned int index = static_cast<unsigned int>(unit);
        if (!m_CacheEnabled || m_ActiveUnit != index)
        {
            glActiveTexture(GLTranslator::ToGL(unit));
            m_ActiveUnit = index;
            RuntimeProfiler::Instance().AddStateChanges();
        }
    }

    void PixelStorei(PixelStoreParam pname, int param) override
    {
        glPixelStorei(GLTranslator::ToGL(pname), param);
    }

    bool SetTextureSwizzle(TextureType target, unsigned int texture, TextureSwizzle swizzle) override
    {
        const GLenum glTarget = GLTranslator::ToGL(target);
        BindTexture(target, texture);
        const GLint red[] = {GL_RED, GL_RED, GL_RED, GL_RED};
        const GLint identity[] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
        glTexParameteriv(glTarget, GL_TEXTURE_SWIZZLE_RGBA,
                         swizzle == TextureSwizzle::RedToRGBA ? red : identity);
        return true;
    }

    const char* GetBackendName() const override
    {
        return "OpenGL";
    }

private:
    static constexpr unsigned int InvalidBinding = (std::numeric_limits<unsigned int>::max)();

    static TextureType NormalizeBindingTarget(TextureType target)
    {
        if (target >= TextureType::CubeMapPositiveX && target <= TextureType::CubeMapNegativeZ)
            return TextureType::TextureCubeMap;
        return target;
    }

    void EnsureActiveUnit()
    {
        if (m_ActiveUnit == InvalidBinding)
        {
            glActiveTexture(GL_TEXTURE0);
            m_ActiveUnit = 0;
            RuntimeProfiler::Instance().AddStateChanges();
        }
    }

    unsigned int m_ActiveUnit = InvalidBinding;
    std::array<std::array<unsigned int, 12>, 32> m_BoundTextures = {};
    bool m_CacheEnabled = true;
};
