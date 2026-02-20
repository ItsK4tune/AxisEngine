#pragma once

#include <scene/scene.h>
#include <interface/physics/i_physics_world.h>
#include <sstream>
#include <fstream>

class PhysicsLoader
{
public:
    static void LoadRigidBody(Scene& scene, entt::entity entity, std::stringstream& ss, IPhysicsWorld& physics, std::ifstream& file);
};
