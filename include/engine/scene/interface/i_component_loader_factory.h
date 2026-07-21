#pragma once

#include <core/logic/yaml_parser.h>
#include <entt/entt.hpp>

class IPhysicsWorld;
class ResourceManager;
struct Scene;

class IComponentLoaderFactory
{
public:
    virtual ~IComponentLoaderFactory() = default;
    virtual void Load(Scene& scene, entt::entity entity, const YAMLNode& node, ResourceManager& res,
                      IPhysicsWorld* phys) = 0;
};
