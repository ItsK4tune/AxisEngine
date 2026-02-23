#include <graphic/renderer/skybox.h>
#include <graphic/core/shader.h>
#include <iostream>
#include <stb_image.h>
#include <interface/graphic/i_buffer_manager.h>
#include <interface/graphic/i_texture_manager.h>
#include <interface/graphic/i_draw_context.h>
#include <utils/logger.h>
#include <interface/graphic/graphics_types.h>

IBufferManager* Skybox::s_BufferManager = nullptr;
ITextureManager* Skybox::s_TextureManager = nullptr;
IDrawContext* Skybox::s_DrawContext = nullptr;

void Skybox::SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_TextureManager = &textureManager;
    s_DrawContext = &drawContext;
}

IBufferManager& Skybox::GetBufferManager()
{
    if (!s_BufferManager) {
        LOGGER_ERROR("Skybox") << "BufferManager not set!";

        throw std::runtime_error("BufferManager not set in Skybox");
    }
    return *s_BufferManager;
}

ITextureManager& Skybox::GetTextureManager()
{
    if (!s_TextureManager) {
        LOGGER_ERROR("Skybox") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in Skybox");
    }
    return *s_TextureManager;
}

IDrawContext& Skybox::GetDrawContext()
{
    if (!s_DrawContext) {
        LOGGER_ERROR("Skybox") << "DrawContext not set!";
        throw std::runtime_error("DrawContext not set in Skybox");
    }
    return *s_DrawContext;
}

Skybox::Skybox()
{
    Init();
}

Skybox::~Skybox()
{
    if (s_BufferManager)
    {
        if (m_VAO) s_BufferManager->DeleteVertexArrays(1, &m_VAO);
        if (m_VBO) s_BufferManager->DeleteBuffers(1, &m_VBO);
    }

    if (s_TextureManager)
    {
        if (m_TextureID) s_TextureManager->DeleteTextures(1, &m_TextureID);
    }
}

void Skybox::Init()
{
    float vertices[] = {
        -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

    if (s_BufferManager)
    {
        auto& bm = GetBufferManager();
        m_VAO = bm.GenVertexArray();
        m_VBO = bm.GenBuffer();

        bm.BindVertexArray(m_VAO);
        bm.BindBuffer(Graphics::BufferType::ArrayBuffer, m_VBO);
        bm.BufferData(Graphics::BufferType::ArrayBuffer, sizeof(vertices), vertices, Graphics::BufferUsage::StaticDraw);

        bm.EnableVertexAttribArray(0);
        bm.VertexAttribPointer(0, 3, Graphics::DataType::Float, false, 3 * sizeof(float), (void *)0);

        bm.BindVertexArray(0);
    }
}

void Skybox::Draw(Shader &shader)
{
    if (s_BufferManager && s_DrawContext)
    {
        GetBufferManager().BindVertexArray(m_VAO);
        GetDrawContext().DrawArrays(Graphics::Primitive::Triangles, 0, 36);
        GetBufferManager().BindVertexArray(0);
    }
}

void Skybox::LoadCubemap(const std::vector<std::string> &faces)
{
    if (!s_TextureManager) return;
    auto& tm = GetTextureManager();

    m_TextureID = tm.GenTexture();
    tm.BindTexture(Graphics::TextureType::TextureCubeMap, m_TextureID);

    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < 6; ++i)
    {
        int width = 0, height = 0, channels = 0;
        unsigned char *data = nullptr;
        Graphics::InternalFormat internalFormat = Graphics::InternalFormat::RGB8;
        Graphics::TextureFormat dataFormat = Graphics::TextureFormat::RGB;

        if (i < faces.size())
        {
            data = stbi_load(faces[i].c_str(), &width, &height, &channels, 0);

            if (data)
            {
                if (width != height)
                {
                    LOGGER_WARN("Skybox") << "Warning: cubemap face not square: " << faces[i] << ", using fallback color.";
                    stbi_image_free(data);
                    data = nullptr;
                }
                else
                {
                    switch (channels)
                    {
                    case 1:
                        dataFormat = Graphics::TextureFormat::Red;
                        const_cast<Graphics::InternalFormat&>(internalFormat) = Graphics::InternalFormat::RGB8;
                        break;
                    case 3:
                        dataFormat = Graphics::TextureFormat::RGB;
                        const_cast<Graphics::InternalFormat&>(internalFormat) = Graphics::InternalFormat::RGB8;
                        break;
                    case 4:
                        dataFormat = Graphics::TextureFormat::RGBA;
                        const_cast<Graphics::InternalFormat&>(internalFormat) = Graphics::InternalFormat::RGBA8;
                        break;
                    default:
                        dataFormat = Graphics::TextureFormat::RGB;
                        const_cast<Graphics::InternalFormat&>(internalFormat) = Graphics::InternalFormat::RGB8;
                        break;
                    }

                    tm.PixelStorei(Graphics::PixelStoreParam::UnpackAlignment, (dataFormat == Graphics::TextureFormat::RGB) ? 1 : 4);
                }
            }
            else
            {
                LOGGER_ERROR("Skybox") << "Failed to load cubemap face: " << faces[i] << ", using fallback color.";
            }
        }

        Graphics::TextureType faceTarget = static_cast<Graphics::TextureType>(static_cast<int>(Graphics::TextureType::CubeMapPositiveX) + i);

        if (!data)
        {
            unsigned char red[3] = {255, 0, 0};
            tm.TexImage2D(faceTarget,
                         0, Graphics::InternalFormat::RGB8, 1, 1, 0, Graphics::TextureFormat::RGB, Graphics::DataType::UnsignedByte, red);
        }
        else
        {
            tm.TexImage2D(faceTarget,
                         0, internalFormat, width, height, 0, dataFormat, Graphics::DataType::UnsignedByte, data);
            stbi_image_free(data);
        }
    }

    tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Linear));
    tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Linear));
    tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
    tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
    tm.TexParameteri(Graphics::TextureType::TextureCubeMap, Graphics::TextureParameter::WrapR, static_cast<int>(Graphics::TextureWrap::ClampToEdge));
}
