#include <graphic/shader_cache.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <glad/glad.h>

namespace fs = std::filesystem;

ShaderCache::ShaderCache()
    : m_CacheDirectory("cache/shaders/")
{
    if (!fs::exists(m_CacheDirectory))
    {
        fs::create_directories(m_CacheDirectory);
    }
}

ShaderCache::~ShaderCache()
{
    for (auto& pair : m_LoadedShaders)
    {
        delete pair.second;
    }
    m_LoadedShaders.clear();
}

Shader* ShaderCache::GetOrCompile(const std::string& name, const std::string& vertPath, const std::string& fragPath)
{
    if (m_LoadedShaders.find(name) != m_LoadedShaders.end())
    {
        return m_LoadedShaders[name];
    }

    std::string cacheKey = GenerateCacheKey(vertPath, fragPath);
    
    Shader* shader = new Shader();
    
    if (CacheExists(cacheKey) && !IsSourceNewer(vertPath, fragPath, cacheKey))
    {
        if (LoadFromCache(cacheKey, shader))
        {
            m_LoadedShaders[name] = shader;
            std::cout << "[ShaderCache] Loaded shader '" << name << "' from cache" << std::endl;
            return shader;
        }
    }
    
    shader->load(vertPath.c_str(), fragPath.c_str());
    SaveToCache(cacheKey, shader);
    
    m_LoadedShaders[name] = shader;
    std::cout << "[ShaderCache] Compiled and cached shader '" << name << "'" << std::endl;
    
    return shader;
}

void ShaderCache::SaveToCache(const std::string& key, const Shader* shader)
{
    std::string cachePath = GetCachePath(key);
    std::ofstream file(cachePath, std::ios::binary);
    
    if (!file.is_open())
    {
        std::cerr << "[ShaderCache] Failed to save cache: " << cachePath << std::endl;
        return;
    }
    
    GLuint programID = shader->getID();
    
    GLint binaryLength = 0;
    glGetProgramiv(programID, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
    
    if (binaryLength == 0)
    {
        std::cerr << "[ShaderCache] No binary available for shader" << std::endl;
        file.close();
        return;
    }
    
    std::vector<unsigned char> binary(binaryLength);
    GLenum binaryFormat = 0;
    
    glGetProgramBinary(programID, binaryLength, nullptr, &binaryFormat, binary.data());
    
    file.write(reinterpret_cast<const char*>(&binaryFormat), sizeof(GLenum));
    file.write(reinterpret_cast<const char*>(&binaryLength), sizeof(GLint));
    file.write(reinterpret_cast<const char*>(binary.data()), binaryLength);
    
    file.close();
}

bool ShaderCache::LoadFromCache(const std::string& key, Shader* shader)
{
    std::string cachePath = GetCachePath(key);
    std::ifstream file(cachePath, std::ios::binary);
    
    if (!file.is_open())
    {
        return false;
    }
    
    GLenum binaryFormat = 0;
    GLint binaryLength = 0;
    
    file.read(reinterpret_cast<char*>(&binaryFormat), sizeof(GLenum));
    file.read(reinterpret_cast<char*>(&binaryLength), sizeof(GLint));
    
    std::vector<unsigned char> binary(binaryLength);
    file.read(reinterpret_cast<char*>(binary.data()), binaryLength);
    
    file.close();
    
    GLuint programID = glCreateProgram();
    glProgramBinary(programID, binaryFormat, binary.data(), binaryLength);
    
    GLint success = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    
    if (!success)
    {
        glDeleteProgram(programID);
        return false;
    }
    
    shader->setID(programID);
    
    return true;
}

void ShaderCache::ClearCache()
{
    if (fs::exists(m_CacheDirectory))
    {
        for (const auto& entry : fs::directory_iterator(m_CacheDirectory))
        {
            fs::remove(entry.path());
        }
    }
    
    std::cout << "[ShaderCache] Cache cleared" << std::endl;
}

void ShaderCache::SetCacheDirectory(const std::string& dir)
{
    m_CacheDirectory = dir;
    if (!fs::exists(m_CacheDirectory))
    {
        fs::create_directories(m_CacheDirectory);
    }
}

std::string ShaderCache::GenerateCacheKey(const std::string& vertPath, const std::string& fragPath)
{
    std::hash<std::string> hasher;
    size_t hash = hasher(vertPath + fragPath);
    
    std::stringstream ss;
    ss << std::hex << hash;
    
    return ss.str();
}

std::string ShaderCache::GetCachePath(const std::string& key)
{
    return m_CacheDirectory + key + ".bin";
}

bool ShaderCache::CacheExists(const std::string& key)
{
    return fs::exists(GetCachePath(key));
}

bool ShaderCache::IsSourceNewer(const std::string& vertPath, const std::string& fragPath, const std::string& cacheKey)
{
    if (!fs::exists(vertPath) || !fs::exists(fragPath))
    {
        return false;
    }
    
    std::string cachePath = GetCachePath(cacheKey);
    if (!fs::exists(cachePath))
    {
        return true;
    }
    
    auto cacheTime = fs::last_write_time(cachePath);
    auto vertTime = fs::last_write_time(vertPath);
    auto fragTime = fs::last_write_time(fragPath);
    
    return (vertTime > cacheTime) || (fragTime > cacheTime);
}
