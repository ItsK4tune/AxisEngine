#pragma once

#include <core/logic/yaml_parser.h>
#include <core/unit/engine_context.h>
#include <scene/logic/scene_serializer.h>

class IPhysicsWorld;
class ResourceManager;
class Scene;

class IComponentLoaderFactory
{
public:
    virtual ~IComponentLoaderFactory() = default;
    virtual void Load(Scene &scene, entt::entity entity, const YAMLNode &node, ResourceManager &res, IPhysicsWorld &phys, EngineContext ctx) = 0;
};