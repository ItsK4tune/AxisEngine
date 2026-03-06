#pragma once

#include <systems/physics/interfaces/i_physics_world.h>
#include <scene/scene.h>
#include <scene/scene_serializer.h>
#include <core/utils/yaml_parser.h>

class PhysicsLoader
{
public:
    static void LoadRigidBody(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics);
};
