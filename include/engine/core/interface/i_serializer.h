#pragma once
#include <string>

template <typename T>
class ISerializer
{
public:
    virtual ~ISerializer() = default;
    virtual bool Serialize(const std::string& filepath, const T& object) = 0;
    virtual bool Deserialize(const std::string& filepath, T& object) = 0;
};
