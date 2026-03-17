#pragma once

#include <render/interface/i_shader_manager.h>
#include <render/logic/shader.h>
#include <resource/logic/resource_cache.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

/**
 * @brief High-level manager for Shader assets.
 * Utilizes ResourceCache for storage and ISahderManager for low-level operations.
 */
class ShaderManager {
public:
    ShaderManager(IShaderManager& lowLevelManager);
    ~ShaderManager() = default;

    /**
     * @brief Loads and compiles a shader program.
     */
    std::shared_ptr<Shader> Load(const std::string& name, 
                                 const std::string& vsPath, 
                                 const std::string& fsPath, 
                                 const std::string& gsPath = "");

    /**
     * @brief Retrieves a shader from the cache.
     */
    std::shared_ptr<Shader> Get(const std::string& name);

    /**
     * @brief Unloads a shader from memory.
     */
    void Unload(const std::string& name);

    /**
     * @brief Reloads a specific shader (hot-reloading).
     */
    void Reload(const std::string& name);

    /**
     * @brief Reloads all currently loaded shaders.
     */
    void ReloadAll();

    /**
     * @brief Clears all cached shaders.
     */
    void Clear();

private:
    IShaderManager& m_LowLevelManager;
    ResourceCache<Shader> m_Cache;

    struct ShaderPaths {
        std::string vs, fs, gs;
    };
    std::unordered_map<std::string, ShaderPaths> m_Paths;
};
