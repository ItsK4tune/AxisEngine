#pragma once

#include <sstream>
#include <string>
#include <vector>

class ILocalizationService
{
public:
    virtual ~ILocalizationService() = default;

    virtual void LoadLanguage(const std::string& path, const std::string& name = "") = 0;
    virtual void SetLanguage(const std::string& language) = 0;
    virtual std::string GetLanguage() const = 0;
    virtual std::string Get(const std::string& key) const = 0;
    virtual std::string Format(const std::string& key, const std::vector<std::string>& arguments) const = 0;

    template <typename... Args>
    std::string GetFormat(const std::string& key, const Args&... arguments) const
    {
        return Format(key, {ToString(arguments)...});
    }

private:
    template <typename T>
    static std::string ToString(const T& value)
    {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }
};
