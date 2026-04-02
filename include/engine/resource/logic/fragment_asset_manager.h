#pragma once

#include <engine/resource/interface/i_asset_manager.h>
#include <engine/resource/type/fragment_asset.h>
#include <unordered_map>
#include <mutex>

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
    std::unordered_map<std::string, std::shared_ptr<FragmentAsset>> m_Fragments;
    std::mutex m_Mutex;
};
