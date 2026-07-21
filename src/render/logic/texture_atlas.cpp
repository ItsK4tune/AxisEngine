#include <render/unit/texture_atlas.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <render/interface/i_texture_manager.h>
#include <resource/logic/stb_image_loader.h>
#include <algorithm>
#include <fstream>
#include <iostream>

ITextureManager* TextureAtlas::s_TextureManager = nullptr;

void TextureAtlas::SetTextureManager(ITextureManager& textureManager)
{
    s_TextureManager = &textureManager;
}

void TextureAtlas::ClearTextureManager()
{
    s_TextureManager = nullptr;
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

        texData.data =
            StbImageLoader::Load(fullPath.c_str(), &texData.width, &texData.height, &texData.channels, 4, false);

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
            StbImageLoader::Free(tex.data);
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

    m_AtlasPixels = std::move(atlasData);

    for (auto& tex : textures)
    {
        StbImageLoader::Free(tex.data);
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
    if (m_AtlasID == 0 || m_Width <= 0 || m_Height <= 0 || m_AtlasPixels.empty())
    {
        LOGGER_ERROR("TextureAtlas") << "Cannot save empty atlas: " << path;
        return false;
    }

    struct AtlasFileHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t width;
        uint32_t height;
        uint32_t regionCount;
        uint32_t pixelDataSize;
    };

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        LOGGER_ERROR("TextureAtlas") << "Failed to open atlas file for writing: " << path;
        return false;
    }

    AtlasFileHeader header;
    header.magic = 0x41585441;
    header.version = 1;
    header.width = static_cast<uint32_t>(m_Width);
    header.height = static_cast<uint32_t>(m_Height);
    header.regionCount = static_cast<uint32_t>(m_Regions.size());
    header.pixelDataSize = static_cast<uint32_t>(m_AtlasPixels.size());

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    for (const auto& [name, region] : m_Regions)
    {
        uint32_t nameSize = static_cast<uint32_t>(name.size());
        file.write(reinterpret_cast<const char*>(&nameSize), sizeof(nameSize));
        file.write(name.data(), name.size());
        file.write(reinterpret_cast<const char*>(&region.uvMin), sizeof(region.uvMin));
        file.write(reinterpret_cast<const char*>(&region.uvMax), sizeof(region.uvMax));
        file.write(reinterpret_cast<const char*>(&region.uvScale), sizeof(region.uvScale));
        file.write(reinterpret_cast<const char*>(&region.uvOffset), sizeof(region.uvOffset));
        file.write(reinterpret_cast<const char*>(&region.width), sizeof(region.width));
        file.write(reinterpret_cast<const char*>(&region.height), sizeof(region.height));
    }
    file.write(reinterpret_cast<const char*>(m_AtlasPixels.data()), m_AtlasPixels.size());

    LOGGER_INFO("TextureAtlas") << "Saved atlas to file: " << path;
    return true;
}

bool TextureAtlas::LoadFromFile(const std::string& path)
{
    if (!s_TextureManager)
        return false;

    struct AtlasFileHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t width;
        uint32_t height;
        uint32_t regionCount;
        uint32_t pixelDataSize;
    };

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        LOGGER_ERROR("TextureAtlas") << "Failed to open atlas file: " << path;
        return false;
    }

    AtlasFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file || header.magic != 0x41585441 || header.version != 1)
    {
        LOGGER_ERROR("TextureAtlas") << "Invalid atlas file: " << path;
        return false;
    }

    std::map<std::string, AtlasRegion> loadedRegions;
    for (uint32_t i = 0; i < header.regionCount; ++i)
    {
        uint32_t nameSize = 0;
        file.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
        std::string name(nameSize, '\0');
        file.read(name.data(), name.size());

        AtlasRegion region;
        region.name = name;
        file.read(reinterpret_cast<char*>(&region.uvMin), sizeof(region.uvMin));
        file.read(reinterpret_cast<char*>(&region.uvMax), sizeof(region.uvMax));
        file.read(reinterpret_cast<char*>(&region.uvScale), sizeof(region.uvScale));
        file.read(reinterpret_cast<char*>(&region.uvOffset), sizeof(region.uvOffset));
        file.read(reinterpret_cast<char*>(&region.width), sizeof(region.width));
        file.read(reinterpret_cast<char*>(&region.height), sizeof(region.height));
        loadedRegions[name] = region;
    }

    std::vector<unsigned char> pixels(header.pixelDataSize);
    file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
    if (!file)
    {
        LOGGER_ERROR("TextureAtlas") << "Atlas file is truncated: " << path;
        return false;
    }

    Clear();

    auto& tm = GetTextureManager();
    m_Width = static_cast<int>(header.width);
    m_Height = static_cast<int>(header.height);
    m_Regions = std::move(loadedRegions);
    m_AtlasPixels = std::move(pixels);

    m_AtlasID = tm.GenTexture();
    tm.BindTexture(TextureType::Texture2D, m_AtlasID);
    tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, m_Width, m_Height, 0, TextureFormat::RGBA,
                  DataType::UnsignedByte, m_AtlasPixels.data());
    tm.GenerateMipmap(TextureType::Texture2D);
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::Repeat));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::Repeat));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                     static_cast<int>(TextureFilter::LinearMipmapLinear));
    tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
    tm.BindTexture(TextureType::Texture2D, 0);

    LOGGER_INFO("TextureAtlas") << "Loaded atlas from file: " << path;
    return true;
}

void TextureAtlas::Clear()
{
    if (m_AtlasID != 0 && s_TextureManager)
    {
        GetTextureManager().DeleteTextures(1, &m_AtlasID);
        m_AtlasID = 0;
    }
    m_Regions.clear();
    m_AtlasPixels.clear();
}

bool TextureAtlas::HasTexture(const std::string& name) const
{
    return m_Regions.find(name) != m_Regions.end();
}
