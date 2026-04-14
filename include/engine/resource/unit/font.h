#pragma once

#include <glm/glm.hpp>
#include <map>
#include <string>

class ITextureManager;

#define GLM_ENABLE_EXPERIMENTAL


struct Character {
    unsigned int textureID;
    glm::ivec2   size;
    glm::ivec2   bearing;
    unsigned int advance;
};

class Font {
public:
    Font();
    ~Font();

    bool Load(const std::string& fontPath, unsigned int fontSize);
    const Character& GetCharacter(unsigned int codepoint) const;
    unsigned int GetFontSize() const { return m_FontSize; }

    static void SetTextureManager(ITextureManager& textureManager);

private:
    unsigned int m_FontSize = 16;
    std::map<unsigned int, Character> m_Characters;
    static ITextureManager* s_TextureManager;
    static ITextureManager& GetTextureManager();
};