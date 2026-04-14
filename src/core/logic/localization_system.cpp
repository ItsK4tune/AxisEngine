#include <core/logic/localization_system.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/service_locator.h>
#include <core/logic/logger.h>
#include <ecs/logic/system_factory.h>
#include <filesystem>

REGISTER_SYSTEM(LocalizationSystem)

LocalizationSystem::LocalizationSystem() {}
LocalizationSystem::~LocalizationSystem() {}

void LocalizationSystem::Initialize()
{
    ServiceLocator::Instance().Register<LocalizationSystem>(this);
    
    // Default to English, then try to load from config if available
    SetLanguage("en");
    
    LOGGER_INFO("LocalizationSystem") << "Initialized with default language: " << m_CurrentLanguage;
}

void LocalizationSystem::Shutdown()
{
    m_Entries.clear();
}

void LocalizationSystem::SetLanguage(const std::string& langCode)
{
    m_CurrentLanguage = langCode;
    m_Entries.clear();
    
    std::string path = "resources/i18n/" + langCode + ".axs";
    if (std::filesystem::exists(path))
    {
        LoadLanguageFile(path);
        LOGGER_INFO("LocalizationSystem") << "Loaded language: " << langCode << " (" << m_Entries.size() << " entries)";
    }
    else
    {
        LOGGER_WARN("LocalizationSystem") << "Language file not found: " << path;
    }
}

std::string LocalizationSystem::Get(const std::string& key) const
{
    auto it = m_Entries.find(key);
    if (it != m_Entries.end())
        return it->second;
    
    return "[MISSING: " + key + "]";
}

void LocalizationSystem::LoadLanguageFile(const std::string& path)
{
    std::vector<YAMLNode> roots = YAMLParser::Parse(path);
    for (const auto& root : roots)
    {
        if (root.key == "axis_localization")
        {
            for (const auto& section : root.children)
            {
                if (section.key == "Entries")
                {
                    FlattenNode(section, "");
                }
            }
        }
    }
}

void LocalizationSystem::FlattenNode(const YAMLNode& node, const std::string& prefix)
{
    for (const auto& child : node.children)
    {
        std::string newKey = prefix.empty() ? child.key : prefix + "." + child.key;
        
        if (child.children.empty())
        {
            m_Entries[newKey] = child.value;
        }
        else
        {
            FlattenNode(child, newKey);
        }
    }
}

std::string LocalizationSystem::FormatString(const std::string& format, const std::vector<std::string>& args) const
{
    std::string result = format;
    for (size_t i = 0; i < args.size(); ++i)
    {
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos)
        {
            result.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }
    return result;
}
