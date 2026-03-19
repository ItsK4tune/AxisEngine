#pragma once

#include <future>
#include <mutex>
#include <render/unit/mesh.h>
#include <string>
#include <unordered_map>
#include <vector>

class ITextureManager;

struct TextureData {
    std::string name;
    std::string path;
    int width, height, nrComponents;
    unsigned char* data = nullptr;
    bool keepCpuData = false;
};

class TextureCache
{
public:
    TextureCache() = default;
    ~TextureCache();

    void LoadTexture(const std::string& name, const std::string& path, bool async = true, bool keepCpuData = false);
    std::shared_ptr<Texture> GetTexture(const std::string& name);
    void UnloadTexture(const std::string& name);
    bool IsTextureLoaded(const std::string& name) const;

    void Update();
    void Clear();

    static void SetTextureManager(ITextureManager* manager) { s_TextureManager = manager; }
    static ITextureManager& GetTextureManager() { return *s_TextureManager; }

    static void SetAsyncEnabled(bool enabled) { s_AsyncEnabled = enabled; }
    static bool IsAsyncEnabled() { return s_AsyncEnabled; }

    static void SetMaxAnisotropy(float anisotropy) { s_MaxAnisotropy = anisotropy; }
    static float GetMaxAnisotropy() { return s_MaxAnisotropy; }

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    std::vector<std::future<TextureData>> m_AsyncLoads;
    std::mutex m_Mutex;
    mutable std::mutex m_CacheMutex;

    static ITextureManager* s_TextureManager;
    static bool s_AsyncEnabled;
    static float s_MaxAnisotropy;
};