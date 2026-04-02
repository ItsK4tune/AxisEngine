#pragma once

#include <core/logic/yaml_parser.h>
#include <physics/interface/i_physics_world.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_serializer.h>

class PhysicsLoader
{
public:
    static void LoadRigidShape(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics);
    static void LoadRigidBody(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics);
    static void LoadCharacterController(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld& physics);
};