#include <resource/font_cache.h>
#include <utils/filesystem.h>
#include <utils/logger.h>

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
        LOGGER_INFO("FontCache") << "Loaded font: " << name;
    }
    else
    {
        LOGGER_ERROR("FontCache") << "Failed to load font: " << path;
    }
}

Font* FontCache::GetFont(const std::string& name)
{
    if (m_Fonts.find(name) != m_Fonts.end())
        return m_Fonts[name].get();
    
    LOGGER_WARN("FontCache") << "Font not found: " << name;
    return nullptr;
}

void FontCache::Clear()
{
    m_Fonts.clear();
}
