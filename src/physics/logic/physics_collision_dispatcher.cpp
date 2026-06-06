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
#include <utility>

PhysicsCollisionDispatcher::PhysicsCollisionDispatcher(Scene& scene, IPhysicsWorld& physics)
    : m_Scene(scene), m_Physics(physics)
{
    LOGGER_INFO("Physics") << "PhysicsCollisionDispatcher initialized";
}

PhysicsCollisionDispatcher::~PhysicsCollisionDispatcher()
{
}

void PhysicsCollisionDispatcher::DispatchEvents()
{
    m_Physics.CollectActiveCollisions(m_CurrentCollisionsList);
    m_currentCollisions.clear();
    if (m_currentCollisions.bucket_count() < m_CurrentCollisionsList.size())
        m_currentCollisions.reserve(m_CurrentCollisionsList.size());

    for (const auto& info : m_CurrentCollisionsList)
    {
        entt::entity eA = info.bodyA;
        entt::entity eB = info.bodyB;

        if (m_Scene.IsValid(eA) && m_Scene.IsValid(eB))
        {
            if (eA > eB)
                std::swap(eA, eB);

            CollisionPair pair{eA, eB};
            if (!m_currentCollisions.insert(pair).second)
                continue;

            bool isTrigger = info.isTrigger;
            bool isStay = m_activeCollisions.count(pair) > 0;

            if (isTrigger)
            {
                EventManager::Instance().Publish(EntityTriggerEvent{static_cast<uint32_t>(eA),
                                                                    static_cast<uint32_t>(eB),
                                                                    isStay ? CollisionEventType::Stay
                                                                           : CollisionEventType::Enter});
            }
            else
            {
                EventManager::Instance().Publish(EntityCollisionEvent{static_cast<uint32_t>(eA),
                                                                      static_cast<uint32_t>(eB),
                                                                      isStay ? CollisionEventType::Stay
                                                                             : CollisionEventType::Enter});
            }
        }
    }

    for (const auto& pair : m_activeCollisions)
    {
        if (m_currentCollisions.count(pair) == 0)
        {
            entt::entity eA = pair.a;
            entt::entity eB = pair.b;

            if (!m_Scene.IsValid(eA) || !m_Scene.IsValid(eB))
                continue;

            bool isTrigger = false;
            if (auto* rbA = m_Scene.TryGetComponent<RigidBodyComponent>(eA))
            {
                if (auto body = rbA->body)
                {
                    if (body->IsTrigger())
                        isTrigger = true;
                }
            }
            if (!isTrigger)
            {
                if (auto* rbB = m_Scene.TryGetComponent<RigidBodyComponent>(eB))
                {
                    if (auto body = rbB->body)
                    {
                        if (body->IsTrigger())
                            isTrigger = true;
                    }
                }
            }

            if (isTrigger)
            {
                EventManager::Instance().Publish(
                    EntityTriggerEvent{static_cast<uint32_t>(eA), static_cast<uint32_t>(eB), CollisionEventType::Exit});
            }
            else
            {
                EventManager::Instance().Publish(EntityCollisionEvent{
                    static_cast<uint32_t>(eA), static_cast<uint32_t>(eB), CollisionEventType::Exit});
            }
        }
    }

    m_activeCollisions.swap(m_currentCollisions);
}
