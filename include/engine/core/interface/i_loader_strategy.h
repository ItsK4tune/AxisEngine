#pragma once
#include <string>

class ILoaderStrategy
{
public:
    virtual ~ILoaderStrategy() = default;

    virtual bool Load(const std::string& path) = 0;

    virtual const char* GetName() const = 0;
};
