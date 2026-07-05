#pragma once

#include <core/interface/i_serializer.h>
#include <core/type/app_config.h>
#include <string>

class ConfigSerializer : public ISerializer<AppConfig>
{
public:
    ConfigSerializer(bool headless = false) : m_Headless(headless) {}

    bool Serialize(const std::string& filepath, const AppConfig& config) override;
    bool Deserialize(const std::string& filepath, AppConfig& config) override;

private:
    bool m_Headless = false;
};
