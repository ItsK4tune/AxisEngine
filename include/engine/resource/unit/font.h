#pragma once

#include <glm/glm.hpp>
#include <map>
#include <string>

class ITextureManager;


struct Character
{
    unsigned int textureID = 0;
    glm::ivec2 size{0};
    glm::ivec2 bearing{0};
    unsigned int advance = 0;
    glm::vec2 uvMin{0.0f};
    glm::vec2 uvMax{0.0f};
};

class Font
{
public:
    Font();
    ~Font();

    bool Load(const std::string& fontPath, unsigned int fontSize);
    const Character& GetCharacter(unsigned int codepoint) const;
    unsigned int GetFontSize() const
    {
        return m_FontSize;
    }
    unsigned int GetAtlasTextureID() const
    {
        return m_AtlasTextureID;
    }

    static void SetTextureManager(ITextureManager& textureManager);
    static void ClearTextureManager();

private:
    unsigned int m_FontSize = 16;
    unsigned int m_AtlasTextureID = 0;
    std::map<unsigned int, Character> m_Characters;
    static ITextureManager* s_TextureManager;
    static ITextureManager& GetTextureManager();
};
