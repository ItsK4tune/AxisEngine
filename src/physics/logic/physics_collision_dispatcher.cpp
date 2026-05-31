#include <physics/logic/physics_collision_dispatcher.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/script_component.h>
#include <physics/interface/i_physics_world.h>
#include <scene/logic/scene.h>
#include <script/logic/physics_scriptable.h>
#include <script/logic/scriptable.h>

PhysicsCollisionDispatcher::PhysicsCollisionDispatcher(Scene& scene, IPhysicsWorld& physics)
    : m_Scene(scene), m_Physics(physics)
{
    LOGGER_INFO("Physics") << "PhysicsCollisionDispatcher initialized";
}

PhysicsCollisionDispatcher::~PhysicsCollisionDispatcher()
{
}

#define GLM_ENABLE_EXPERIMENTAL
#include <script/logic/physics_scriptable.h>
#include <glm/glm.hpp>
#include <unordered_set>

void PhysicsCollisionDispatcher::DispatchEvents()
{
    auto currentCollisionsList = m_Physics.GetActiveCollisions();

    std::unordered_set<CollisionPair, CollisionPairHash> currentCollisions;
    currentCollisions.reserve(currentCollisionsList.size());

    for (const auto& info : currentCollisionsList)
    {
        entt::entity eA = info.bodyA;
        entt::entity eB = info.bodyB;

        if (m_Scene.registry.valid(eA) && m_Scene.registry.valid(eB))
        {
            if (eA > eB)
                std::swap(eA, eB);

            currentCollisions.insert({eA, eB});

            bool isTrigger = info.isTrigger;
            bool isStay = m_activeCollisions.count({eA, eB}) > 0;

            auto Notify = [&](entt::entity target, entt::entity other, bool trigger, bool stay) {
                if (!m_Scene.registry.valid(target) || !m_Scene.registry.valid(other))
                    return;

                if (trigger)
                {
                    EventManager::Instance().Publish(
                        EntityTriggerEvent{static_cast<uint32_t>(target), static_cast<uint32_t>(other),
                                           stay ? CollisionEventType::Stay : CollisionEventType::Enter});
                }
                else
                {
                    EventManager::Instance().Publish(
                        EntityCollisionEvent{static_cast<uint32_t>(target), static_cast<uint32_t>(other),
                                             stay ? CollisionEventType::Stay : CollisionEventType::Enter});
                }
            };

            Notify(eA, eB, isTrigger, isStay);
            Notify(eB, eA, isTrigger, isStay);
        }
    }

    for (const auto& pair : m_activeCollisions)
    {
        if (currentCollisions.count(pair) == 0)
        {
            entt::entity eA = pair.a;
            entt::entity eB = pair.b;

            if (!m_Scene.registry.valid(eA) || !m_Scene.registry.valid(eB))
                continue;

            bool isTrigger = false;
            if (m_Scene.registry.all_of<RigidBodyComponent>(eA))
            {
                if (auto body = m_Scene.registry.get<RigidBodyComponent>(eA).body)
                    if (body->IsTrigger())
                        isTrigger = true;
            }
            if (!isTrigger && m_Scene.registry.all_of<RigidBodyComponent>(eB))
            {
                if (auto body = m_Scene.registry.get<RigidBodyComponent>(eB).body)
                    if (body->IsTrigger())
                        isTrigger = true;
            }

            auto NotifyExit = [&](entt::entity target, entt::entity other, bool trigger) {
                if (!m_Scene.registry.valid(target) || !m_Scene.registry.valid(other))
                    return;

                if (trigger)
                {
                    EventManager::Instance().Publish(EntityTriggerEvent{
                        static_cast<uint32_t>(target), static_cast<uint32_t>(other), CollisionEventType::Exit});
                }
                else
                {
                    EventManager::Instance().Publish(EntityCollisionEvent{
                        static_cast<uint32_t>(target), static_cast<uint32_t>(other), CollisionEventType::Exit});
                }
            };

            NotifyExit(eA, eB, isTrigger);
            NotifyExit(eB, eA, isTrigger);
        }
    }

    m_activeCollisions = currentCollisions;
}
