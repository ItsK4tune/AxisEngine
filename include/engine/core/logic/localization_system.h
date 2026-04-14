#pragma once

#include <ecs/interface/i_base_system.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <sstream>

class LocalizationSystem : public IBaseSystem
{
public:
    LocalizationSystem();
    virtual ~LocalizationSystem();

    // IBaseSystem implementation
    virtual SystemCategory GetCategory() const override { return SystemCategory::Core; }
    virtual SystemRequirement GetRequirements() const override { return SystemRequirement::None; }
    virtual void Initialize() override;
    virtual void Shutdown() override;
    
    virtual bool IsEnabled() const override { return m_Enabled; }
    virtual void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    virtual std::string GetName() const override { return "LocalizationSystem"; }

    // Localization API
    void SetLanguage(const std::string& langCode);
    std::string Get(const std::string& key) const;
    
    template<typename... Args>
    std::string GetFormat(const std::string& key, Args... args) const
    {
        std::string raw = Get(key);
        std::vector<std::string> strArgs = { ToString(args)... };
        return FormatString(raw, strArgs);
    }

private:
    void LoadLanguageFile(const std::string& path);
    void FlattenNode(const struct YAMLNode& node, const std::string& prefix);
    
    std::string FormatString(const std::string& format, const std::vector<std::string>& args) const;
    
    template<typename T>
    std::string ToString(const T& val) const
    {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    std::unordered_map<std::string, std::string> m_Entries;
    std::string m_CurrentLanguage = "en";
    bool m_Enabled = true;
};
