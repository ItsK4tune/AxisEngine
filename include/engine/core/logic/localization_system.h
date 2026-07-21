#pragma once

#include <core/interface/i_base_system.h>
#include <core/interface/i_localization_service.h>
#include <string>
#include <unordered_map>
#include <vector>

class LocalizationSystem : public IBaseSystem, public ILocalizationService
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
    void LoadLanguage(const std::string& path, const std::string& name = "") override;
    void SetLanguage(const std::string& langCode) override;
    std::string GetLanguage() const override;
    std::string Get(const std::string& key) const override;
    std::string Format(const std::string& key, const std::vector<std::string>& arguments) const override;

private:
    void FlattenNode(const struct YAMLNode& node, const std::string& prefix,
                     std::unordered_map<std::string, std::string>& outEntries);

    std::string FormatString(const std::string& format, const std::vector<std::string>& args) const;

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_Languages;
    std::string m_CurrentLanguage;
    bool m_Enabled = true;
};
