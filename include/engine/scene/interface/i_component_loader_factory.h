#pragma once

#include <core/logic/yaml_parser.h>
#include <memory>
#include <scene/logic/scene_serializer.h>

class IPhysicsWorld;
class ResourceManager;
class Scene;

class IComponentLoaderFactory
{
public:
    virtual ~IComponentLoaderFactory() = default;
    virtual void Load(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res, IPhysicsWorld *phys) = 0;
};