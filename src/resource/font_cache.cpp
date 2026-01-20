#include <resource/font_cache.h>
#include <utils/filesystem.h>
#include <iostream>

FontCache::FontCache()
{
}

FontCache::~FontCache()
{
    Clear();
}

void FontCache::LoadFont(const std::string& name, const std::string& path, unsigned int fontSize)
{
    auto font = std::make_unique<Font>();
    if (font->Load(FileSystem::getPath(path), fontSize))
    {
        m_Fonts[name] = std::move(font);
        std::cout << "[FontCache] Loaded font: " << name << std::endl;
    }
    else
    {
        std::cout << "[FontCache] Failed to load font: " << path << std::endl;
    }
}

Font* FontCache::GetFont(const std::string& name)
{
    if (m_Fonts.find(name) != m_Fonts.end())
        return m_Fonts[name].get();
    
    std::cerr << "[FontCache] Font not found: " << name << std::endl;
    return nullptr;
}

void FontCache::Clear()
{
    m_Fonts.clear();
}
