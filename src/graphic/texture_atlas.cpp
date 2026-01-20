#include <graphic/texture_atlas.h>
#include <utils/filesystem.h>
#include <stb_image.h>
#include <algorithm>
#include <iostream>
#include <fstream>

TextureAtlas::TextureAtlas()
    : m_AtlasID(0), m_Width(0), m_Height(0)
{
}

TextureAtlas::~TextureAtlas()
{
    Clear();
}

bool TextureAtlas::CreateAtlas(const std::vector<std::string>& texturePaths,
                               int atlasWidth, int atlasHeight)
{
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
            std::cerr << "[TextureAtlas] Failed to load texture: " << path << std::endl;
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
        std::cerr << "[TextureAtlas] Failed to pack textures" << std::endl;
        for (auto& tex : textures)
        {
            stbi_image_free(tex.data);
        }
        return false;
    }
    
    std::vector<unsigned char> atlasData(m_Width * m_Height * 4, 0);
    
    for (size_t i = 0; i < textures.size(); i++)
    {
        BlitTexture(atlasData.data(), textures[i], rects[i]);
        
        AtlasRegion region;
        region.name = textureNames[i];
        region.uvMin = glm::vec2((float)rects[i].x / m_Width, (float)rects[i].y / m_Height);
        region.uvMax = glm::vec2((float)(rects[i].x + rects[i].width) / m_Width,
                                 (float)(rects[i].y + rects[i].height) / m_Height);
        region.uvScale = glm::vec2((float)rects[i].width / m_Width, (float)rects[i].height / m_Height);
        region.uvOffset = region.uvMin;
        region.width = rects[i].width;
        region.height = rects[i].height;
        
        m_Regions[textureNames[i]] = region;
    }
    
    glGenTextures(1, &m_AtlasID);
    glBindTexture(GL_TEXTURE_2D, m_AtlasID);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlasData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    for (auto& tex : textures)
    {
        stbi_image_free(tex.data);
    }
    
    std::cout << "[TextureAtlas] Created atlas with " << textures.size() << " textures" << std::endl;
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
        indexed.push_back({ (int)i, textures[i].width, textures[i].height });
    }
    
    std::sort(indexed.begin(), indexed.end(), [](const IndexedTexture& a, const IndexedTexture& b) {
        return a.height > b.height;
    });
    
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
            std::cerr << "[TextureAtlas] Atlas size too small for " << textures.size() << " textures" << std::endl;
            return false;
        }
        
        outRects[tex.index] = { currentX, currentY, tex.width, tex.height };
        currentX += tex.width;
        shelfHeight = (std::max)(shelfHeight, tex.height);
    }
    
    return true;
}

void TextureAtlas::BlitTexture(unsigned char* atlasData, const TextureData& texture, const Rect& rect)
{
    for (int y = 0; y < rect.height; y++)
    {
        for (int x = 0; x < rect.width; x++)
        {
            int atlasIndex = ((rect.y + y) * m_Width + (rect.x + x)) * 4;
            int texIndex = (y * texture.width + x) * 4;
            
            atlasData[atlasIndex + 0] = texture.data[texIndex + 0];
            atlasData[atlasIndex + 1] = texture.data[texIndex + 1];
            atlasData[atlasIndex + 2] = texture.data[texIndex + 2];
            atlasData[atlasIndex + 3] = texture.data[texIndex + 3];
        }
    }
}

TextureAtlas::AtlasRegion TextureAtlas::GetRegion(const std::string& textureName) const
{
    auto it = m_Regions.find(textureName);
    if (it != m_Regions.end())
        return it->second;
    
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
    std::cout << "[TextureAtlas] Save to file not yet implemented" << std::endl;
    return false;
}

bool TextureAtlas::LoadFromFile(const std::string& path)
{
    std::cout << "[TextureAtlas] Load from file not yet implemented" << std::endl;
    return false;
}

void TextureAtlas::Clear()
{
    if (m_AtlasID != 0)
    {
        glDeleteTextures(1, &m_AtlasID);
        m_AtlasID = 0;
    }
    m_Regions.clear();
}

bool TextureAtlas::HasTexture(const std::string& name) const
{
    return m_Regions.find(name) != m_Regions.end();
}
