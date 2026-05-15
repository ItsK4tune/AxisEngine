#pragma once

#include <render/unit/skybox.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>
#include <vector>


class SkyboxManager : public IAssetManager<Skybox> {
public:
     SkyboxManager() = default;
    ~SkyboxManager() = default;

    
    std::shared_ptr<Skybox> Load(const std::string& path) override {
        std::vector<std::string> faces = {
            path + "/right.jpg", path + "/left.jpg",
            path + "/top.jpg",   path + "/bottom.jpg",
            path + "/front.jpg", path + "/back.jpg"
        };
        return Load(path, faces);
    }

    
    std::shared_ptr<Skybox> Load(const std::string& name, const std::vector<std::string>& faces);

    
    std::shared_ptr<Skybox> Get(const std::string& nameOrPath) override;

    
    void Unload(const std::string& nameOrPath) override;

    
    void Clear() override;
    
    std::vector<std::string> GetAllNames() const { return m_Cache.GetAllNames(); }

private:
    ResourceCache<Skybox> m_Cache;
};
