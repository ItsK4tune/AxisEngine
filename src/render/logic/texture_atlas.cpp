#include <render/unit/texture_atlas.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <render/interface/i_texture_manager.h>
#include <stb/stb_image.h>
#include <algorithm>
#include <fstream>
#include <iostream>

ITextureManager* TextureAtlas::s_TextureManager = nullptr;

void TextureAtlas::SetTextureManager(ITextureManager& textureManager)
{
    s_TextureManager = &textureManager;
}

ITextureManager& TextureAtlas::GetTextureManager()
{
    if (!s_TextureManager)
    {
        LOGGER_ERROR("TextureAtlas") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in TextureAtlas");
    }
    return *s_TextureManager;
}

TextureAtlas::TextureAtlas() : m_AtlasID(0), m_Width(0), m_Height(0)
{
}

TextureAtlas::~TextureAtlas()
{
    Clear();
}

bool TextureAtlas::CreateAtlas(const std::vector<std::string>& texturePaths, int atlasWidth, int atlasHeight)
{
    if (!s_TextureManager)
        return false;
    auto& tm = GetTextureManager();

    m_Width = atlasWidth;
    m_Height = atlasHeight;

    std::vector<TextureData> textures;
    std::vector<std::string> textureNames;

    for (const auto& path : texturePaths)
    {
        TextureData texData;
        std::string fullPath = FileSystem::getPath(path);

        stbi_set_flip_vertically_on_load(false);
        texData.data = stbi_load(fullPath.c_str(), &texData.width, &texData.height, &texData.channels, 4);

        if (!texData.data)
        {
            LOGGER_ERROR("TextureAtlas") << "Failed to load Texture: " << path;
            continue;
        }

        textures.push_back(texData);

        size_t lastSlash = path.find_last_of("/\\");
        std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        textureNames.push_back(name);
    }

    std::vector<Rect> rects;
    if (!PackTextures(textures, rects))
    {
        LOGGER_ERROR("TextureAtlas") << "Failed to pack textures";
        for (auto& tex : textures)
        {
            stbi_image_free(tex.data);
        }
        return false;
    }

    std::vector<unsigned char> atlasData(m_Width * m_Height * 4, 0);

    for (size_t i = 0; i < textures.size(); i++)
    {
        for (int y = 0; y < rects[i].height; y++)
        {
            for (int x = 0; x < rects[i].width; x++)
            {
                int atlasIndex = ((rects[i].y + y) * m_Width + (rects[i].x + x)) * 4;
                int texIndex = (y * textures[i].width + x) * 4;

                atlasData[atlasIndex + 0] = textures[i].data[texIndex + 0];
                atlasData[atlasIndex + 1] = textures[i].data[texIndex + 1];
                atlasData[atlasIndex + 2] = textures[i].data[texIndex + 2];
                atlasData[atlasIndex + 3] = textures[i].data[texIndex + 3];
            }
        }

        AtlasRegion region;
        region.name = textureNames[i];
        region.uvMin = glm::vec2((float)rects[i].x / m_Width, (float)rects[i].y / m_Height);
        region.uvMax =
            glm::vec2((float)(rects[i].x + rects[i].width) / m_Width, (float)(rects[i].y + rects[i].height) / m_Height);
        region.uvScale = glm::vec2((float)rects[i].width / m_Width, (float)rects[i].height / m_Height);
        region.uvOffset = region.uvMin;
        region.width = rects[i].width;
        region.height = rects[i].height;

        m_Regions[textureNames[i]] = region;
    }

    m_AtlasID = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_AtlasID);

    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, m_Width, m_Height, 0, TextureFormat::RGBA,
                  DataType::UnsignedByte, atlasData.data());
    tm.GenerateMipmap(TextureType::Texture2D);

    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::Repeat));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::Repeat));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                     static_cast<int>(TextureFilter::LinearMipmapLinear));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));

    tm.BindTexture(TextureType::Texture2D, 0);

    for (auto& tex : textures)
    {
        stbi_image_free(tex.data);
    }

    LOGGER_INFO("TextureAtlas") << "Created atlas with " << textures.size() << " textures";
    return true;
}

bool TextureAtlas::PackTextures(const std::vector<TextureData>& textures, std::vector<Rect>& outRects)
{
    struct IndexedTexture
    {
        int index;
        int width, height;
    };

    std::vector<IndexedTexture> indexed;
    for (size_t i = 0; i < textures.size(); i++)
    {
        indexed.push_back({(int)i, textures[i].width, textures[i].height});
    }

    std::sort(indexed.begin(), indexed.end(),
              [](const IndexedTexture& a, const IndexedTexture& b) { return a.height > b.height; });

    outRects.resize(textures.size());

    int currentY = 0;
    int currentX = 0;
    int shelfHeight = 0;

    for (const auto& tex : indexed)
    {
        if (currentX + tex.width > m_Width)
        {
            currentY += shelfHeight;
            currentX = 0;
            shelfHeight = 0;
        }

        if (currentY + tex.height > m_Height)
        {
            LOGGER_ERROR("TextureAtlas") << "Atlas size too small for " << textures.size() << " textures";
            return false;
        }

        outRects[tex.index] = {currentX, currentY, tex.width, tex.height};
        currentX += tex.width;
        shelfHeight = (std::max)(shelfHeight, tex.height);
    }

    return true;
}

TextureAtlas::AtlasRegion TextureAtlas::GetRegion(const std::string& textureName) const
{
    auto it = m_Regions.find(textureName);
    if (it != m_Regions.end())
        return it->second;

    LOGGER_WARN("TextureAtlas") << "Region not found: " << textureName;
    return AtlasRegion();
}

glm::vec4 TextureAtlas::TransformUV(const std::string& textureName, const glm::vec2& uv)
{
    auto region = GetRegion(textureName);
    glm::vec2 transformedUV = uv * region.uvScale + region.uvOffset;
    return glm::vec4(transformedUV, region.uvScale);
}

bool TextureAtlas::SaveToFile(const std::string& path)
{
    LOGGER_WARN("TextureAtlas") << "Save to file not yet implemented";
    return false;
}

bool TextureAtlas::LoadFromFile(const std::string& path)
{
    LOGGER_WARN("TextureAtlas") << "Load from file not yet implemented";
    return false;
}

void TextureAtlas::Clear()
{
    if (m_AtlasID != 0 && s_TextureManager)
    {
        GetTextureManager().DeleteTextures(1, &m_AtlasID);
        m_AtlasID = 0;
    }
    m_Regions.clear();
}

bool TextureAtlas::HasTexture(const std::string& name) const
{
    return m_Regions.find(name) != m_Regions.end();
}
