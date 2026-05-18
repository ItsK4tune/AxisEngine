#pragma once

#include <memory>
#include <string>
#include <vector>

template <typename T>
class IAssetManager
{
public:
    virtual ~IAssetManager() = default;

    virtual std::shared_ptr<T> Load(const std::string& path) = 0;

    virtual std::shared_ptr<T> Get(const std::string& nameOrPath) = 0;

    virtual void Unload(const std::string& nameOrPath) = 0;

    virtual void Clear() = 0;

    virtual std::vector<std::string> GetAllNames() const
    {
        return {};
    }

    virtual void Update(float dt)
    {
    }

    virtual void Initialize()
    {
    }
};
