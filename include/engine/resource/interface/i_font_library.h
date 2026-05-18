#pragma once

#include <memory>
#include <string>

class Font;

class IFontLibrary
{
public:
    virtual ~IFontLibrary() = default;
    virtual void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize) = 0;
    virtual void UnloadFont(const std::string& name) = 0;
    virtual std::shared_ptr<Font> GetFont(const std::string& name) = 0;
};
