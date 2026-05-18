#pragma once

#include <memory>
#include <string>
#include <vector>

class Skybox;

class ISkyboxLibrary
{
public:
    virtual ~ISkyboxLibrary() = default;
    virtual void LoadSkybox(const std::string& name, const std::vector<std::string>& faces) = 0;
    virtual void UnloadSkybox(const std::string& name) = 0;
    virtual std::shared_ptr<Skybox> GetSkybox(const std::string& name) = 0;
};
