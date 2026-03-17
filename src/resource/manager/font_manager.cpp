#include <resource/manager/font_manager.h>
#include <core/logic/logger.h>

std::shared_ptr<Font> FontManager::Load(const std::string& name, const std::string& path, unsigned int fontSize) {
    if (auto existing = m_Cache.Get(name)) return existing;

    auto font = std::make_shared<Font>();
    if (font->Load(path, fontSize)) {
        m_Cache.Add(name, font);
        LOGGER_INFO("FontManager") << "Loaded font: " << name << " (Size: " << fontSize << ")";
        return font;
    }
    
    LOGGER_ERROR("FontManager") << "Failed to load font: " << path;
    return nullptr;
}

std::shared_ptr<Font> FontManager::Get(const std::string& name) {
    return m_Cache.Get(name);
}

void FontManager::Unload(const std::string& name) {
    m_Cache.Remove(name);
}
