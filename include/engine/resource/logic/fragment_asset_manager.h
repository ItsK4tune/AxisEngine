#pragma once

#include <engine/resource/interface/i_asset_manager.h>
#include <engine/resource/type/fragment_asset.h>
#include <engine/resource/logic/resource_cache.h>

class FragmentAssetManager : public IAssetManager<FragmentAsset>
{
public:
    FragmentAssetManager() = default;
    virtual ~FragmentAssetManager() = default;

    std::shared_ptr<FragmentAsset> Load(const std::string& path) override;
    std::shared_ptr<FragmentAsset> Get(const std::string& nameOrPath) override;
    void Unload(const std::string& nameOrPath) override;
    void Clear() override;

private:
    ResourceCache<FragmentAsset> m_Cache;
};
