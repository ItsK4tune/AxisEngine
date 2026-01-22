#include <physic/physics_collision_dispatcher.h>
#include <scene/scene.h>
#include <physic/physic_world.h>
#include <ecs/component.h>
#include <script/scriptable.h>

PhysicsCollisionDispatcher::PhysicsCollisionDispatcher(Scene& scene, PhysicsWorld& physics)
    : m_Scene(scene), m_Physics(physics)
{
}

PhysicsCollisionDispatcher::~PhysicsCollisionDispatcher()
{
}

void PhysicsCollisionDispatcher::DispatchEvents()
{
    btDiscreteDynamicsWorld *world = m_Physics.GetWorld();
    if (!world) return;

    int numManifolds = world->getDispatcher()->getNumManifolds();

    std::unordered_set<CollisionPair, CollisionPairHash> currentCollisions;
    currentCollisions.reserve(numManifolds);

    for (int i = 0; i < numManifolds; i++)
    {
        btPersistentManifold *contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject *obA = contactManifold->getBody0();
        const btCollisionObject *obB = contactManifold->getBody1();

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
            entt::entity eA = (entt::entity)(uintptr_t)obA->getUserPointer();
            entt::entity eB = (entt::entity)(uintptr_t)obB->getUserPointer();

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
                    if (m_Scene.registry.all_of<ScriptComponent>(target))
                    {
                        auto &s = m_Scene.registry.get<ScriptComponent>(target);
                        if (s.instance)
                        {
                            if (trigger)
                            {
                                if (!stay)
                                    s.instance->OnTriggerEnter(other);
                                else
                                    s.instance->OnTriggerStay(other);
                            }
                            else
                            {
                                if (!stay)
                                    s.instance->OnCollisionEnter(other);
                                else
                                    s.instance->OnCollisionStay(other);
                            }
                        }
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

            bool isTrigger = false;
            if (m_Scene.registry.valid(eA) && m_Scene.registry.all_of<RigidBodyComponent>(eA))
            {
                if (auto *body = m_Scene.registry.get<RigidBodyComponent>(eA).body)
                    if (body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
                        isTrigger = true;
            }
            if (!isTrigger && m_Scene.registry.valid(eB) && m_Scene.registry.all_of<RigidBodyComponent>(eB))
            {
                if (auto *body = m_Scene.registry.get<RigidBodyComponent>(eB).body)
                    if (body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
                        isTrigger = true;
            }

            auto NotifyExit = [&](entt::entity target, entt::entity other, bool trigger)
            {
                if (m_Scene.registry.valid(target) && m_Scene.registry.all_of<ScriptComponent>(target))
                {
                    auto &s = m_Scene.registry.get<ScriptComponent>(target);
                    if (s.instance)
                    {
                        if (trigger)
                            s.instance->OnTriggerExit(other);
                        else
                            s.instance->OnCollisionExit(other);
                    }
                }
            };

            NotifyExit(eA, eB, isTrigger);
            NotifyExit(eB, eA, isTrigger);
        }
    }

    m_activeCollisions = currentCollisions;
}
