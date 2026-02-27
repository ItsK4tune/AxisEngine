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
    auto font = std::make_shared<Font>();
    std::lock_guard<std::mutex> lock(m_Mutex);
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

std::shared_ptr<Font> FontCache::GetFont(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Fonts.find(name) != m_Fonts.end())
        return m_Fonts[name];

    LOGGER_WARN("FontCache") << "Font not found: " << name;
    return nullptr;
}

void FontCache::Remove(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Fonts.erase(name);
}

void FontCache::Clear()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Fonts.clear();
}
