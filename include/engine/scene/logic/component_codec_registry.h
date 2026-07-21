#pragma once

#include <scene/interface/i_component_codec_registry.h>
#include <mutex>
#include <unordered_map>

class ComponentCodecRegistry final : public IComponentCodecRegistry
{
public:
    ~ComponentCodecRegistry() override;

    bool RegisterLoader(const std::string& owner, const std::string& type,
                        std::shared_ptr<IComponentLoaderFactory> factory) override;
    bool RegisterLoader(const std::string& owner, const std::string& type, ComponentLoaderCallback callback) override;
    bool RegisterSerializer(const std::string& owner, const std::string& type,
                            std::shared_ptr<IComponentSerializerFactory> factory) override;
    bool RegisterSerializer(const std::string& owner, const std::string& type,
                            ComponentSerializerCallback callback) override;
    size_t UnregisterOwner(const std::string& owner) override;

private:
    std::mutex m_Mutex;
    std::unordered_map<std::string, std::string> m_LoaderOwners;
    std::unordered_map<std::string, std::string> m_SerializerOwners;
};
