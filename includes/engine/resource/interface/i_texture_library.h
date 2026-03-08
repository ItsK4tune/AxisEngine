#pragma once

#include <memory>
#include <string>

class Texture;

class ITextureLibrary
{
public:
    virtual ~ITextureLibrary() = default;
    virtual void LoadTexture(const std::string& name, const std::string& path, bool async = true) = 0;
    virtual void UnloadTexture(const std::string& name) = 0;
    virtual std::shared_ptr<Texture> GetTexture(const std::string& name) = 0;
};