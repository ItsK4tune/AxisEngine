#pragma once

#include <graphic/font.h>
#include <string>
#include <map>
#include <memory>

class FontCache
{
public:
    FontCache();
    ~FontCache();

    void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize);
    Font* GetFont(const std::string& name);
    
    void Clear();

private:
    std::map<std::string, std::unique_ptr<Font>> m_Fonts;
};
