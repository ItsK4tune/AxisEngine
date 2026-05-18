#pragma once

#include <resource/unit/mesh.h>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

class ITextureManager;

#define GLM_ENABLE_EXPERIMENTAL

class TextureAtlas
{
public:
    struct AtlasRegion
    {
        std::string name;
        glm::vec2 uvMin;
        glm::vec2 uvMax;
        glm::vec2 uvScale;
        glm::vec2 uvOffset;
        int width, height;
    };

    TextureAtlas();
    ~TextureAtlas();

    bool CreateAtlas(const std::vector<std::string>& texturePaths, int atlasWidth = 2048, int atlasHeight = 2048);

    unsigned int GetAtlasTexture() const
    {
        return m_AtlasID;
    }
    AtlasRegion GetRegion(const std::string& textureName) const;

    glm::vec4 TransformUV(const std::string& textureName, const glm::vec2& uv);

    bool SaveToFile(const std::string& path);
    bool LoadFromFile(const std::string& path);

    void Clear();
    bool HasTexture(const std::string& name) const;

    static void SetTextureManager(ITextureManager& textureManager);

private:
    unsigned int m_AtlasID;
    int m_Width, m_Height;
    std::map<std::string, AtlasRegion> m_Regions;

    struct Rect
    {
        int x, y, width, height;
    };

    struct TextureData
    {
        unsigned char* data;
        int width, height, channels;
    };

    bool PackTextures(const std::vector<TextureData>& textures, std::vector<Rect>& outRects);

    static ITextureManager* s_TextureManager;
    static ITextureManager& GetTextureManager();
};
