#include <ecs/unit/physics_components.h>
#include <physics/logic/physics_collision_dispatcher.h>
#include <scene/logic/scene.h>
#include <physics/interface/i_physics_world.h>
#include <ecs/unit/core_components.h>
#include <script/logic/scriptable.h>
#include <script/logic/physics_scriptable.h>
#include <core/logic/event_types.h>
#include <core/logic/event_system.h>
#include <core/logic/logger.h>

PhysicsCollisionDispatcher::PhysicsCollisionDispatcher(Scene &scene, IPhysicsWorld &physics)
    : m_Scene(scene), m_Physics(physics)
{
    LOGGER_INFO("Physics") << "PhysicsCollisionDispatcher initialized";
}

PhysicsCollisionDispatcher::~PhysicsCollisionDispatcher()
{
}

#include <physics/strategy/bullet/bullet_physics_world.h>

void PhysicsCollisionDispatcher::DispatchEvents()
{
    BulletPhysicsWorld *bulletWorld = dynamic_cast<BulletPhysicsWorld *>(&m_Physics);
    if (!bulletWorld)
        return;

    btDiscreteDynamicsWorld *world = bulletWorld->GetRawWorld();
    if (!world)
        return;

    int numManifolds = world->getDispatcher()->getNumManifolds();

    std::unordered_set<CollisionPair, CollisionPairHash> currentCollisions;
    currentCollisions.reserve(numManifolds);

    for (int i = 0; i < numManifolds; i++)
    {
        btPersistentManifold *contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
        if (!contactManifold)
            continue;

        const btCollisionObject *obA = contactManifold->getBody0();
        const btCollisionObject *obB = contactManifold->getBody1();
        if (!obA || !obB)
            continue;

        bool hasCollision = false;
        for (int j = 0; j < contactManifold->getNumContacts(); j++)
        {
            if (contactManifold->getContactPoint(j).getDistance() < 0.1f)
            {
                hasCollision = true;
                break;
            }
        }

        if (hasCollision)
        {
            void *ptrA = obA->getUserPointer();
            void *ptrB = obB->getUserPointer();

            if (!ptrA || !ptrB)
            {
                LOGGER_WARN("PhysicsCollisionDispatcher") << "Collision detected but missing user pointers!";
                continue;
            }

            entt::entity eA = (entt::entity)(uintptr_t)ptrA;
            entt::entity eB = (entt::entity)(uintptr_t)ptrB;

            if (m_Scene.registry.valid(eA) && m_Scene.registry.valid(eB))
            {
                if (eA > eB)
                    std::swap(eA, eB);

                currentCollisions.insert({eA, eB});

                bool isTrigger = (obA->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) ||
                                 (obB->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE);

                bool isStay = m_activeCollisions.count({eA, eB}) > 0;

                auto Notify = [&](entt::entity target, entt::entity other, bool trigger, bool stay)
                {
                    if (!m_Scene.registry.valid(target) || !m_Scene.registry.valid(other))
                        return;

                    if (m_Scene.registry.all_of<ScriptComponent>(target))
                    {
                        auto &s = m_Scene.registry.get<ScriptComponent>(target);
                        if (s.instance)
                        {
                            if (trigger)
                            {
                                if (auto* ps = dynamic_cast<PhysicsScriptable*>(s.instance.get()))
                                {
                                    if (!stay)
                                        ps->OnTriggerEnter(other);
                                    else
                                        ps->OnTriggerStay(other);
                                }
                            }
                            else
                            {
                                if (auto* ps = dynamic_cast<PhysicsScriptable*>(s.instance.get()))
                                {
                                    if (!stay)
                                        ps->OnCollisionEnter(other);
                                    else
                                        ps->OnCollisionStay(other);
                                }
                            }
                        }
                    }

                    if (trigger)
                    {
                        EventSystem::Instance().Publish(EntityTriggerEvent{target, other, stay ? CollisionEventType::Stay : CollisionEventType::Enter});
                    }
                    else
                    {
                        EventSystem::Instance().Publish(EntityCollisionEvent{target, other, stay ? CollisionEventType::Stay : CollisionEventType::Enter});
                    }
                };

                Notify(eA, eB, isTrigger, isStay);
                Notify(eB, eA, isTrigger, isStay);
            }
        }
    }

    for (const auto &pair : m_activeCollisions)
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

            auto NotifyExit = [&](entt::entity target, entt::entity other, bool trigger)
            {
                if (!m_Scene.registry.valid(target) || !m_Scene.registry.valid(other))
                    return;

                if (m_Scene.registry.all_of<ScriptComponent>(target))
                {
                    auto &s = m_Scene.registry.get<ScriptComponent>(target);
                    if (s.instance)
                    {
                        if (trigger)
                        {
                            if (auto* ps = dynamic_cast<PhysicsScriptable*>(s.instance.get()))
                                ps->OnTriggerExit(other);
                        }
                        else
                        {
                            if (auto* ps = dynamic_cast<PhysicsScriptable*>(s.instance.get()))
                                ps->OnCollisionExit(other);
                        }
                    }
                }

                if (trigger)
                {
                    EventSystem::Instance().Publish(EntityTriggerEvent{target, other, CollisionEventType::Exit});
                }
                else
                {
                    EventSystem::Instance().Publish(EntityCollisionEvent{target, other, CollisionEventType::Exit});
                }
            };

            NotifyExit(eA, eB, isTrigger);
            NotifyExit(eB, eA, isTrigger);
        }
    }

    m_activeCollisions = currentCollisions;
}
