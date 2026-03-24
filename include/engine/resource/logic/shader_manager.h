#ifndef AXIS_SHADER_MANAGER_H
#define AXIS_SHADER_MANAGER_H

#pragma once

#include <render/interface/i_shader_manager.h>
#include <resource/unit/shader.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>


class ShaderManager : public IAssetManager<Shader> {
public:
    ShaderManager(IShaderManager& lowLevelManager);
    ~ShaderManager() = default;

    
    std::shared_ptr<Shader> Load(const std::string& path) override {
        return Load(path, path + ".vs", path + ".fs");
    }

    
    std::shared_ptr<Shader> Load(const std::string& name, 
                                 const std::string& vsPath, 
                                 const std::string& fsPath, 
                                 const std::string& gsPath = "");

    
    std::shared_ptr<Shader> Get(const std::string& nameOrPath) override;

    
    void Unload(const std::string& nameOrPath) override;

    
    void Clear() override;

    
    void Reload(const std::string& name);

    
    void ReloadAll();

private:
    IShaderManager& m_LowLevelManager;
    ResourceCache<Shader> m_Cache;

    struct ShaderPaths {
        std::string vs, fs, gs;
    };
    std::unordered_map<std::string, ShaderPaths> m_Paths;
};

#endif
