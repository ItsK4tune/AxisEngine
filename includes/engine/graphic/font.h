#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

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

private:
    std::map<char, Character> Characters;
};