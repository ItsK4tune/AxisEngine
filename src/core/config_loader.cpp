#include <core/config_loader.h>
#include <fstream>
#include <sstream>
#include <algorithm>

// Minimal JSON Parser helper
// Handles simple "key": value or "key": "value" lines.
// Does NOT support nested objects or arrays or multi-line values robustly.

static std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\",{}");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r\",{}");
    return str.substr(first, (last - first + 1));
}

static std::string ExtractValue(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) return "";
    return Trim(line.substr(colonPos + 1));
}

static std::string ExtractKey(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) return "";
    return Trim(line.substr(0, colonPos));
}

AppConfig ConfigLoader::Load(const std::string& path)
{
    AppConfig config;
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ConfigLoader] Could not open config file: " << path << ". Using defaults." << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::string key = ExtractKey(line);
        std::string value = ExtractValue(line);
        
        if (key.empty()) continue;

        if (key == "title") config.title = value;
        else if (key == "width") config.width = std::stoi(value);
        else if (key == "height") config.height = std::stoi(value);
        else if (key == "windowMode") config.windowMode = std::stoi(value);
        else if (key == "vsync") config.vsync = (value == "true");
        else if (key == "monitorIndex") config.monitorIndex = std::stoi(value);
        else if (key == "refreshRate") config.refreshRate = std::stoi(value);
        else if (key == "frameRateLimit") config.frameRateLimit = std::stoi(value);
        else if (key == "shadowsEnabled") config.shadowsEnabled = (value == "true");
        else if (key == "cullFaceEnabled") config.cullFaceEnabled = (value == "true");
        else if (key == "depthTestEnabled") config.depthTestEnabled = (value == "true");
        else if (key == "audioDevice") config.audioDevice = value;
        else if (key == "iconPath") config.iconPath = value;
    }
    
    return config;
}
