#pragma once

#include <resource/unit/animation.h>
#include <resource/logic/resource_cache.h>
#include <resource/interface/i_asset_manager.h>
#include <string>
#include <memory>


class ModelManager;


class AnimationManager : public IAssetManager<Animation> {
public:
    AnimationManager(ModelManager& modelManager);
    ~AnimationManager() = default;

    
    std::shared_ptr<Animation> Load(const std::string& path) override {
        return Load(path, path, "");
    }

    
    std::shared_ptr<Animation> Load(const std::string& name, const std::string& path, const std::string& modelName);

    
    std::shared_ptr<Animation> Get(const std::string& nameOrPath) override;

    
    void Unload(const std::string& nameOrPath) override;

    
    void Clear() override;
    std::vector<std::string> GetAllNames() const { return m_Cache.GetAllNames(); }

private:
    ModelManager& m_ModelManager;
    ResourceCache<Animation> m_Cache;
};
