#pragma once

#include <ecs/interface/i_base_system.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class LocalizationSystem : public IBaseSystem
{
public:
    LocalizationSystem();
    virtual ~LocalizationSystem();

    // IBaseSystem implementation
    virtual SystemCategory GetCategory() const override
    {
        return SystemCategory::Core;
    }
    virtual SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::None;
    }
    virtual void Initialize() override;
    virtual void Shutdown() override;

    virtual bool IsEnabled() const override
    {
        return m_Enabled;
    }
    virtual void SetEnabled(bool enabled) override
    {
        m_Enabled = enabled;
    }
    virtual std::string GetName() const override
    {
        return "LocalizationSystem";
    }

    // Localization API
    void LoadLanguage(const std::string& path, const std::string& name = "");
    void SetLanguage(const std::string& langCode);
    std::string GetLanguage() const;
    std::string Get(const std::string& key) const;

    template <typename... Args>
    std::string GetFormat(const std::string& key, Args... args) const
    {
        std::string raw = Get(key);
        std::vector<std::string> strArgs = {ToString(args)...};
        return FormatString(raw, strArgs);
    }

private:
    void FlattenNode(const struct YAMLNode& node, const std::string& prefix,
                     std::unordered_map<std::string, std::string>& outEntries);

    std::string FormatString(const std::string& format, const std::vector<std::string>& args) const;

    template <typename T>
    std::string ToString(const T& val) const
    {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_Languages;
    std::string m_CurrentLanguage;
    bool m_Enabled = true;
};
