#pragma once

#include <core/scene.h>
#include <physic/physic_world.h>
#include <sstream>
#include <fstream>

class PhysicsLoader
{
public:
    static void LoadRigidBody(Scene& scene, entt::entity entity, std::stringstream& ss, PhysicsWorld& physics, std::ifstream& file);
};
