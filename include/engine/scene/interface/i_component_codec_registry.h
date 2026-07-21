#pragma once

#include <core/logic/yaml_parser.h>
#include <scene/interface/i_component_loader_factory.h>
#include <scene/interface/i_component_serializer_factory.h>
#include <entt/entt.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

class IPhysicsWorld;
class ResourceManager;
struct Scene;

using ComponentLoaderCallback =
    std::function<void(Scene&, entt::entity, const YAMLNode&, ResourceManager&, IPhysicsWorld*)>;
using ComponentSerializerCallback = std::function<bool(const entt::registry&, entt::entity, YAMLNode&)>;

class IComponentCodecRegistry
{
public:
    virtual ~IComponentCodecRegistry() = default;

    virtual bool RegisterLoader(const std::string& owner, const std::string& type,
                                std::shared_ptr<IComponentLoaderFactory> factory) = 0;
    virtual bool RegisterLoader(const std::string& owner, const std::string& type,
                                ComponentLoaderCallback callback) = 0;
    virtual bool RegisterSerializer(const std::string& owner, const std::string& type,
                                    std::shared_ptr<IComponentSerializerFactory> factory) = 0;
    virtual bool RegisterSerializer(const std::string& owner, const std::string& type,
                                    ComponentSerializerCallback callback) = 0;
    virtual size_t UnregisterOwner(const std::string& owner) = 0;
};
