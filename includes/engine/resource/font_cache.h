#pragma once

#include <graphic/renderer/font.h>
#include <string>
#include <map>
#include <memory>

class FontCache
{
public:
    FontCache();
    ~FontCache();

    void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize);
    std::shared_ptr<Font> GetFont(const std::string& name);
    void Remove(const std::string& name) { m_Fonts.erase(name); }
    void Clear();

private:
    std::map<std::string, std::shared_ptr<Font>> m_Fonts;
};
