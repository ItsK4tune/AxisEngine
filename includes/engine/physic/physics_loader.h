#pragma once

#include <scene/scene.h>
#include <interface/physics/i_physics_world.h>
#include <scene/scene_serializer.h>

class PhysicsLoader
{
public:
    static void LoadRigidBody(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics);
};
