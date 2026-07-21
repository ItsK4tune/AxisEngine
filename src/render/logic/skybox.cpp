#include <render/unit/skybox.h>
#include <core/logic/logger.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/type/graphics_types.h>
#include <resource/logic/stb_image_loader.h>
#include <resource/unit/shader.h>
#include <iostream>

IBufferManager* Skybox::s_BufferManager = nullptr;
ITextureManager* Skybox::s_TextureManager = nullptr;
IDrawContext* Skybox::s_DrawContext = nullptr;

void Skybox::SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_TextureManager = &textureManager;
    s_DrawContext = &drawContext;
}

void Skybox::ClearManagers()
{
    s_BufferManager = nullptr;
    s_TextureManager = nullptr;
    s_DrawContext = nullptr;
}

IBufferManager& Skybox::GetBufferManager()
{
    if (!s_BufferManager)
    {
        LOGGER_ERROR("Skybox") << "BufferManager not set!";

        throw std::runtime_error("BufferManager not set in Skybox");
    }
    return *s_BufferManager;
}

ITextureManager& Skybox::GetTextureManager()
{
    if (!s_TextureManager)
    {
        LOGGER_ERROR("Skybox") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in Skybox");
    }
    return *s_TextureManager;
}

IDrawContext& Skybox::GetDrawContext()
{
    if (!s_DrawContext)
    {
        LOGGER_ERROR("Skybox") << "DrawContext not set!";
        throw std::runtime_error("DrawContext not set in Skybox");
    }
    return *s_DrawContext;
}

Skybox::Skybox()
{
    Initialize();
}

Skybox::~Skybox()
{
    if (s_BufferManager)
    {
        try
        {
            if (m_VAO)
                s_BufferManager->DeleteVertexArrays(1, &m_VAO);
            if (m_VBO)
                s_BufferManager->DeleteBuffers(1, &m_VBO);
        }
        catch (...)
        {
            LOGGER_ERROR("Skybox") << "Destructor: CRASH during buffer deletion";
        }
    }

    if (s_TextureManager)
    {
        try
        {
            if (m_TextureID)
                s_TextureManager->DeleteTextures(1, &m_TextureID);
        }
        catch (...)
        {
            LOGGER_ERROR("Skybox") << "Destructor: CRASH during Texture deletion";
        }
    }
}

void Skybox::Initialize()
{
    float vertices[] = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

                        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

                        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

                        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

                        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

                        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    if (s_BufferManager)
    {
        auto& bm = GetBufferManager();
        m_VAO = bm.GenVertexArray();
        m_VBO = bm.GenBuffer();

        bm.BindVertexArray(m_VAO);
        bm.BindBuffer(BufferType::ArrayBuffer, m_VBO);
        bm.BufferData(BufferType::ArrayBuffer, sizeof(vertices), vertices, BufferUsage::StaticDraw);

        bm.EnableVertexAttribArray(0);
        bm.VertexAttribPointer(0, 3, DataType::Float, false, 3 * sizeof(float), (void*)0);

        bm.BindVertexArray(0);
    }
}

void Skybox::Draw(Shader& shader)
{
    if (s_BufferManager && s_DrawContext)
    {
        GetBufferManager().BindVertexArray(m_VAO);
        GetDrawContext().DrawArrays(Primitive::Triangles, 0, 36);
        GetBufferManager().BindVertexArray(0);
    }
}

#include <core/logic/filesystem.h>

bool Skybox::LoadCubemap(const std::vector<std::string>& faces)
{
    if (!s_TextureManager)
        return false;
    auto& tm = GetTextureManager();

    m_TextureID = tm.GenTexture();
    tm.BindTexture(TextureType::TextureCubeMap, m_TextureID);

    bool allFacesLoaded = faces.size() == 6;
    for (unsigned int i = 0; i < 6; ++i)
    {
        int width = 0, height = 0, channels = 0;
        unsigned char* data = nullptr;
        InternalFormat internalFormat = InternalFormat::RGB8;
        TextureFormat dataFormat = TextureFormat::RGB;

        if (i < faces.size())
        {
            std::string fullPath = FileSystem::getPath(faces[i]);
            data = StbImageLoader::Load(fullPath.c_str(), &width, &height, &channels, 0, false);

            if (data)
            {
                if (width != height)
                {
                    LOGGER_WARN("Skybox")
                        << "Warning: cubemap face not square: " << faces[i] << ", using fallback color.";
                    StbImageLoader::Free(data);
                    data = nullptr;
                }
                else
                {
                    switch (channels)
                    {
                        case 1:
                            dataFormat = TextureFormat::Red;
                            const_cast<InternalFormat&>(internalFormat) = InternalFormat::RGB8;
                            break;
                        case 3:
                            dataFormat = TextureFormat::RGB;
                            const_cast<InternalFormat&>(internalFormat) = InternalFormat::RGB8;
                            break;
                        case 4:
                            dataFormat = TextureFormat::RGBA;
                            const_cast<InternalFormat&>(internalFormat) = InternalFormat::RGBA8;
                            break;
                        default:
                            dataFormat = TextureFormat::RGB;
                            const_cast<InternalFormat&>(internalFormat) = InternalFormat::RGB8;
                            break;
                    }

                    tm.PixelStorei(PixelStoreParam::UnpackAlignment, (dataFormat == TextureFormat::RGB) ? 1 : 4);
                }
            }
            else
            {
                LOGGER_ERROR("Skybox") << "Failed to load cubemap face: " << faces[i] << ", using fallback color.";
            }
        }

        TextureType faceTarget = static_cast<TextureType>(static_cast<int>(TextureType::CubeMapPositiveX) + i);

        if (!data)
        {
            allFacesLoaded = false;
            unsigned char red[3] = {255, 0, 0};
            tm.TexImage2D(faceTarget, 0, InternalFormat::RGB8, 1, 1, 0, TextureFormat::RGB, DataType::UnsignedByte,
                          red);
        }
        else
        {
            tm.TexImage2D(faceTarget, 0, internalFormat, width, height, 0, dataFormat, DataType::UnsignedByte, data);
            StbImageLoader::Free(data);
        }
    }

    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Linear));
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapS, static_cast<int>(TextureWrap::ClampToEdge));
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapT, static_cast<int>(TextureWrap::ClampToEdge));
    tm.TexParameteri(TextureType::TextureCubeMap, TextureParameter::WrapR, static_cast<int>(TextureWrap::ClampToEdge));
    return allFacesLoaded;
}
