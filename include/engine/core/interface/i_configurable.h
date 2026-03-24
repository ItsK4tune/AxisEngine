#pragma once
#include <string>
#include <variant>


using ConfigValue = std::variant<int, float, bool, std::string>;


class IConfigurable {
public:
    virtual ~IConfigurable() = default;

    
    virtual void OnConfigChanged(const std::string& key, const ConfigValue& value) = 0;
};
