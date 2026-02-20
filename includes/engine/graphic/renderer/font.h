#pragma once

#include <glm/glm.hpp>
#include <map>
#include <string>

class ITextureManager;

struct Character {
    unsigned int TextureID;
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
};

class Font {
public:
    Font();
    ~Font();

    bool Load(const std::string& fontPath, unsigned int fontSize);
    const Character& GetCharacter(char c) const;

    static void SetTextureManager(ITextureManager& textureManager);

private:
    std::map<char, Character> Characters;
    static ITextureManager* s_TextureManager;
    static ITextureManager& GetTextureManager();
};
