#ifndef AXIS_SHADER_MANAGER_H
#define AXIS_SHADER_MANAGER_H

#pragma once

#include <render/interface/i_shader_manager.h>
#include <render/unit/shader.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

/**
 * @brief High-level manager for Shader assets.
 * Utilizes ResourceCache for storage and IShaderManager for low-level operations.
 */
class ShaderManager : public IAssetManager<Shader> {
public:
    ShaderManager(IShaderManager& lowLevelManager);
    ~ShaderManager() = default;

    /**
     * @brief Loads and compiles a shader program from a base path (IAssetManager implementation).
     * @param path Base path (e.g., "shaders/standard" looks for .vs, .fs).
     */
    std::shared_ptr<Shader> Load(const std::string& path) override {
        return Load(path, path + ".vs", path + ".fs");
    }

    /**
     * @brief Loads and compiles a shader program with explicit paths.
     */
    std::shared_ptr<Shader> Load(const std::string& name, 
                                 const std::string& vsPath, 
                                 const std::string& fsPath, 
                                 const std::string& gsPath = "");

    /**
     * @brief Retrieves a shader from the cache.
     */
    std::shared_ptr<Shader> Get(const std::string& nameOrPath) override;

    /**
     * @brief Unloads a shader from memory.
     */
    void Unload(const std::string& nameOrPath) override;

    /**
     * @brief Clears all cached shaders.
     */
    void Clear() override;

    /**
     * @brief Reloads a specific shader.
     */
    void Reload(const std::string& name);

    /**
     * @brief Reloads all shaders in the cache.
     */
    void ReloadAll();

private:
    IShaderManager& m_LowLevelManager;
    ResourceCache<Shader> m_Cache;

    struct ShaderPaths {
        std::string vs, fs, gs;
    };
    std::unordered_map<std::string, ShaderPaths> m_Paths;
};

#endif // AXIS_SHADER_MANAGER_H
