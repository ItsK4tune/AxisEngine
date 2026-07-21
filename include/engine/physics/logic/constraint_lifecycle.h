#pragma once

#include <entt/entt.hpp>
#include <memory>

class IConstraint;
class IPhysicsWorld;
struct RigidBodyComponent;

namespace PhysicsConstraintLifecycle
{
void Track(const std::shared_ptr<IConstraint>& constraint, RigidBodyComponent& first, RigidBodyComponent& second);
void RemoveAll(entt::registry& registry, IPhysicsWorld& physicsWorld, RigidBodyComponent& rigidBody);
}  // namespace PhysicsConstraintLifecycle
