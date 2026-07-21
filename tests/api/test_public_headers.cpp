#include "test_framework.h"

#include <axis_plugin.h>
#include <axis_sdk.h>

namespace
{
class PublicApiSystem final : public IBaseSystem, public IOptimizationConfigurable
{
public:
    bool IsEnabled() const override
    {
        return enabled;
    }
    void SetEnabled(bool value) override
    {
        enabled = value;
    }
    std::string GetName() const override
    {
        return "PublicApiSystem";
    }
    void ApplyOptimizationConfig(const OptimizationConfig& config) override
    {
        particleBudget = config.particleMaxSpawnPerFrame;
    }
    int particleBudget = 0;

private:
    bool enabled = true;
};

class PublicLocalization final : public ILocalizationService
{
public:
    void LoadLanguage(const std::string&, const std::string& name) override
    {
        language = name;
    }
    void SetLanguage(const std::string& value) override
    {
        language = value;
    }
    std::string GetLanguage() const override
    {
        return language;
    }
    std::string Get(const std::string& key) const override
    {
        return key;
    }
    std::string Format(const std::string& key, const std::vector<std::string>& arguments) const override
    {
        return arguments.empty() ? key : key + ":" + arguments.front();
    }

private:
    std::string language;
};
}  // namespace

AXIS_TEST_CASE("Stable and plugin umbrella headers expose replaceable contracts")
{
    PublicApiSystem system;
    PublicLocalization localization;
    AppBuilder providers;
    providers.WithWindowFactory([] { return std::unique_ptr<IWindow>{}; });
    OptimizationConfig optimization;
    optimization.particleMaxSpawnPerFrame = 55;
    system.ApplyOptimizationConfig(optimization);

    AXIS_CHECK(system.GetName() == "PublicApiSystem");
    AXIS_CHECK(system.particleBudget == 55);
    AXIS_CHECK(localization.Format("public", {"contract"}) == "public:contract");
    AXIS_CHECK(providers.GetCapabilities().customWindow);
}
