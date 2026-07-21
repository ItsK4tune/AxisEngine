#pragma once

#include <core/logic/yaml_parser.h>
#include <physics/interface/i_physics_world.h>
#include <scene/logic/scene.h>

class PhysicsLoader
{
public:
    static void LoadRigidShape(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld* physics);
    static void LoadRigidBody(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld* physics);
    static void LoadCharacterController(Scene& scene, entt::entity entity, const YAMLNode& node,
                                        IPhysicsWorld* physics);

    static void LoadRigidShape(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics)
    {
        LoadRigidShape(scene, entity, node, &physics);
    }
    static void LoadRigidBody(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics)
    {
        LoadRigidBody(scene, entity, node, &physics);
    }
    static void LoadCharacterController(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics)
    {
        LoadCharacterController(scene, entity, node, &physics);
    }
};
