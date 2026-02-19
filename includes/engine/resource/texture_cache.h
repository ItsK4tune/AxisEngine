#pragma once

#include <graphic/geometry/mesh.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <future>
#include <mutex>

class ITextureManager;

struct TextureData {
    std::string name;
    std::string path;
    int width, height, nrComponents;
    unsigned char* data = nullptr;
};

class TextureCache
{
public:
    TextureCache() = default;
    ~TextureCache();

    void LoadTexture(const std::string& name, const std::string& path, bool async = true);
    Texture* GetTexture(const std::string& name);
    void UnloadTexture(const std::string& name);
    bool IsTextureLoaded(const std::string& name) const;
    
    void Update();
    void Clear();

    static void SetTextureManager(ITextureManager* manager) { s_TextureManager = manager; }
    static ITextureManager& GetTextureManager() { return *s_TextureManager; }

private:
    std::unordered_map<std::string, Texture> m_Textures;
    std::vector<std::future<TextureData>> m_AsyncLoads;
    std::mutex m_Mutex;
    
    static ITextureManager* s_TextureManager;
};
