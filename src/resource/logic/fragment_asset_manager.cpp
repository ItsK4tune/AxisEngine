#include <engine/resource/logic/fragment_asset_manager.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>

std::shared_ptr<FragmentAsset> FragmentAssetManager::Load(const std::string& path)
{
    if (m_Cache.Has(path))
        return m_Cache.Get(path);

    std::string fullPath = FileSystem::getPath(path);
    auto roots = YAMLParser::Parse(fullPath);
    
    if (roots.empty())
    {
        LOGGER_ERROR("FragmentAssetManager") << "Failed to parse AXS fragment: " << fullPath;
        return nullptr;
    }

    auto asset = std::make_shared<FragmentAsset>();
    asset->path = path;
    asset->rootNodes = roots;
    
    m_Cache.Add(path, asset);
    
    LOGGER_INFO("FragmentAssetManager") << "Loaded fragment: " << path;
    return asset;
}

std::shared_ptr<FragmentAsset> FragmentAssetManager::Get(const std::string& nameOrPath)
{
    return m_Cache.Get(nameOrPath);
}

void FragmentAssetManager::Unload(const std::string& nameOrPath)
{
    m_Cache.Remove(nameOrPath);
    LOGGER_INFO("FragmentAssetManager") << "Unloaded fragment: " << nameOrPath;
}

void FragmentAssetManager::Clear()
{
    m_Cache.Clear();
    LOGGER_INFO("FragmentAssetManager") << "Cleared all fragments";
}
