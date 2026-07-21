#include <physics/logic/constraint_lifecycle.h>
#include <ecs/unit/physics_components.h>
#include <physics/interface/i_constraint.h>
#include <physics/interface/i_physics_world.h>
#include <algorithm>
#include <unordered_set>

namespace PhysicsConstraintLifecycle
{
namespace
{
void TrackOne(const std::shared_ptr<IConstraint>& constraint, RigidBodyComponent& rigidBody)
{
    if (std::find(rigidBody.constraints.begin(), rigidBody.constraints.end(), constraint) ==
        rigidBody.constraints.end())
        rigidBody.constraints.push_back(constraint);
}
}  // namespace

void Track(const std::shared_ptr<IConstraint>& constraint, RigidBodyComponent& first, RigidBodyComponent& second)
{
    if (!constraint)
        return;
    TrackOne(constraint, first);
    TrackOne(constraint, second);
}

void RemoveAll(entt::registry& registry, IPhysicsWorld& physicsWorld, RigidBodyComponent& rigidBody)
{
    const auto constraints = rigidBody.constraints;
    std::unordered_set<const IConstraint*> removed;
    for (const auto& constraint : constraints)
    {
        if (!constraint || !removed.insert(constraint.get()).second)
            continue;

        physicsWorld.RemoveConstraint(constraint);
        auto bodies = registry.view<RigidBodyComponent>();
        for (auto entity : bodies)
        {
            auto& other = bodies.get<RigidBodyComponent>(entity);
            std::erase(other.constraints, constraint);
        }
    }
    rigidBody.constraints.clear();
}
}  // namespace PhysicsConstraintLifecycle
