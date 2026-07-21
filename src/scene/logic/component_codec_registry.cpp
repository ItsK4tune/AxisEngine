#include <scene/logic/component_codec_registry.h>
#include <scene/logic/component_loader.h>
#include <unordered_set>
#include <utility>

ComponentCodecRegistry::~ComponentCodecRegistry()
{
    std::unordered_set<std::string> owners;
    {
        std::lock_guard lock(m_Mutex);
        for (const auto& [type, owner] : m_LoaderOwners) owners.insert(owner);
        for (const auto& [type, owner] : m_SerializerOwners) owners.insert(owner);
    }
    for (const auto& owner : owners) UnregisterOwner(owner);
}

bool ComponentCodecRegistry::RegisterLoader(const std::string& owner, const std::string& type,
                                            std::shared_ptr<IComponentLoaderFactory> factory)
{
    if (owner.empty() || type.empty() || !factory)
        return false;
    std::lock_guard lock(m_Mutex);
    ComponentLoader::RegisterLoader(type, std::move(factory));
    m_LoaderOwners[type] = owner;
    return true;
}

bool ComponentCodecRegistry::RegisterLoader(const std::string& owner, const std::string& type,
                                            ComponentLoaderCallback callback)
{
    if (owner.empty() || type.empty() || !callback)
        return false;
    std::lock_guard lock(m_Mutex);
    ComponentLoader::RegisterLoader(type, std::move(callback));
    m_LoaderOwners[type] = owner;
    return true;
}

bool ComponentCodecRegistry::RegisterSerializer(const std::string& owner, const std::string& type,
                                                std::shared_ptr<IComponentSerializerFactory> factory)
{
    if (owner.empty() || type.empty() || !factory)
        return false;
    std::lock_guard lock(m_Mutex);
    ComponentLoader::RegisterSerializer(type, std::move(factory));
    m_SerializerOwners[type] = owner;
    return true;
}

bool ComponentCodecRegistry::RegisterSerializer(const std::string& owner, const std::string& type,
                                                ComponentSerializerCallback callback)
{
    if (owner.empty() || type.empty() || !callback)
        return false;
    std::lock_guard lock(m_Mutex);
    ComponentLoader::RegisterSerializer(type, std::move(callback));
    m_SerializerOwners[type] = owner;
    return true;
}

size_t ComponentCodecRegistry::UnregisterOwner(const std::string& owner)
{
    if (owner.empty())
        return 0;

    size_t removed = 0;
    std::lock_guard lock(m_Mutex);
    for (auto it = m_LoaderOwners.begin(); it != m_LoaderOwners.end();)
    {
        if (it->second == owner)
        {
            ComponentLoader::UnregisterLoader(it->first);
            it = m_LoaderOwners.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }
    for (auto it = m_SerializerOwners.begin(); it != m_SerializerOwners.end();)
    {
        if (it->second == owner)
        {
            ComponentLoader::UnregisterSerializer(it->first);
            it = m_SerializerOwners.erase(it);
            ++removed;
        }
        else
        {
            ++it;
        }
    }
    return removed;
}
