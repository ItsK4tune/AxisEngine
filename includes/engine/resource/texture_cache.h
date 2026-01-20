#pragma once

#include <graphic/mesh.h>
#include <string>
#include <map>
#include <vector>
#include <future>
#include <mutex>

struct TextureData {
    std::string name;
    std::string path;
    int width, height, nrComponents;
    unsigned char* data = nullptr;
};

class TextureCache
{
public:
    TextureCache();
    ~TextureCache();

    void LoadTexture(const std::string& name, const std::string& path, bool async = true);
    Texture* GetTexture(const std::string& name);
    
    void Update();
    void Clear();

private:
    std::map<std::string, Texture> m_Textures;
    std::vector<std::future<TextureData>> m_AsyncLoads;
    std::mutex m_Mutex;
};
