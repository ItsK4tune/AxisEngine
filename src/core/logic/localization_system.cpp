#include <core/logic/localization_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/logic/yaml_parser.h>
#include <filesystem>

LocalizationSystem::LocalizationSystem()
{
}
LocalizationSystem::~LocalizationSystem()
{
}

void LocalizationSystem::Initialize()
{
    ServiceLocator::Instance().Register<LocalizationSystem>(this);
    LOGGER_INFO("LocalizationSystem") << "Initialized (no default language loaded)";
}

void LocalizationSystem::Shutdown()
{
    m_Languages.clear();
}

void LocalizationSystem::LoadLanguage(const std::string& path, const std::string& name)
{
    std::string langName = name.empty() ? path : name;

    if (!std::filesystem::exists(path))
    {
        LOGGER_WARN("LocalizationSystem") << "Language file not found: " << path;
        return;
    }

    std::vector<YAMLNode> roots = YAMLParser::Parse(path);
    std::unordered_map<std::string, std::string> entries;
    for (const auto& root : roots)
    {
        if (root.key == "axis_localization")
        {
            for (const auto& section : root.children)
            {
                if (section.key == "Entries")
                {
                    FlattenNode(section, "", entries);
                }
            }
        }
    }

    m_Languages[langName] = std::move(entries);

    if (m_CurrentLanguage.empty())
    {
        m_CurrentLanguage = langName;
    }

    LOGGER_INFO("LocalizationSystem") << "Loaded language: " << langName << " (" << m_Languages[langName].size()
                                      << " entries)";
}

void LocalizationSystem::SetLanguage(const std::string& langCode)
{
    m_CurrentLanguage = langCode;
    LOGGER_INFO("LocalizationSystem") << "Language active set to: " << m_CurrentLanguage;
}

std::string LocalizationSystem::GetLanguage() const
{
    return m_CurrentLanguage;
}

std::string LocalizationSystem::Get(const std::string& key) const
{
    auto langIt = m_Languages.find(m_CurrentLanguage);
    if (langIt != m_Languages.end())
    {
        auto entryIt = langIt->second.find(key);
        if (entryIt != langIt->second.end())
            return entryIt->second;
    }

    return "[MISSING: " + key + "]";
}

void LocalizationSystem::FlattenNode(const YAMLNode& node, const std::string& prefix,
                                     std::unordered_map<std::string, std::string>& outEntries)
{
    for (const auto& child : node.children)
    {
        std::string newKey = prefix.empty() ? child.key : prefix + "." + child.key;

        if (child.children.empty())
        {
            outEntries[newKey] = child.value;
        }
        else
        {
            FlattenNode(child, newKey, outEntries);
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
