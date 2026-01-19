#pragma once

#include <graphic/shader.h>
#include <string>
#include <unordered_map>
#include <vector>

class ShaderCache
{
public:
    ShaderCache();
    ~ShaderCache();

    Shader* GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath);
    
    void SaveToCache(const std::string& key, const Shader* shader);
    bool LoadFromCache(const std::string& key, Shader* shader);
    
    void ClearCache();
    
    void SetCacheDirectory(const std::string& dir);
    std::string GetCacheDirectory() const { return m_CacheDirectory; }

private:
    std::string GenerateCacheKey(const std::string& vertPath, const std::string& fragPath);
    std::string GetCachePath(const std::string& key);
    
    bool CacheExists(const std::string& key);
    bool IsSourceNewer(const std::string& vertPath, const std::string& fragPath, const std::string& cacheKey);
    
    std::string m_CacheDirectory;
    std::unordered_map<std::string, Shader*> m_LoadedShaders;
};
