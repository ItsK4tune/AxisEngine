#include <engine/resource/logic/fragment_asset_manager.h>
#include <core/logic/yaml_parser.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>

std::shared_ptr<FragmentAsset> FragmentAssetManager::Load(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_Fragments.find(path) != m_Fragments.end())
        return m_Fragments[path];

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
    
    m_Fragments[path] = asset;
    
    LOGGER_INFO("FragmentAssetManager") << "Loaded fragment: " << path;
    return asset;
}

std::shared_ptr<FragmentAsset> FragmentAssetManager::Get(const std::string& nameOrPath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Fragments.find(nameOrPath) != m_Fragments.end())
        return m_Fragments[nameOrPath];
    return nullptr;
}

void FragmentAssetManager::Unload(const std::string& nameOrPath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Fragments.erase(nameOrPath);
    LOGGER_INFO("FragmentAssetManager") << "Unloaded fragment: " << nameOrPath;
}

void FragmentAssetManager::Clear()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Fragments.clear();
    LOGGER_INFO("FragmentAssetManager") << "Cleared all fragments";
}
