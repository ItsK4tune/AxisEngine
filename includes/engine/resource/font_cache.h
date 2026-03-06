#pragma once

#include <graphics/renderer/font.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>

class FontCache
{
public:
    FontCache();
    ~FontCache();

    void LoadFont(const std::string& name, const std::string& path, unsigned int fontSize);
    std::shared_ptr<Font> GetFont(const std::string& name);
    void Remove(const std::string& name);
    void Clear();

private:
    std::map<std::string, std::shared_ptr<Font>> m_Fonts;
    mutable std::mutex m_Mutex;
};
