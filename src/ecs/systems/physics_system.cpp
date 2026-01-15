#include <ecs/system.h>
#include <core/scriptable.h>
#include <physic/physic_world.h>
#include <iostream>

void PhysicsSystem::Update(Scene &scene, PhysicsWorld &physicsWorld, float dt)
{
    physicsWorld.Update(dt);

    auto view = scene.registry.view<RigidBodyComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto &rb = view.get<RigidBodyComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);

        if (rb.body)
        {
            if (rb.body->isKinematicObject() || !rb.body->isActive())
                continue;

            btTransform trans;
            if (rb.body->getMotionState())
                rb.body->getMotionState()->getWorldTransform(trans);
            else
                trans = rb.body->getWorldTransform();

            transform.position = BulletGLMHelpers::convert(trans.getOrigin());
            transform.rotation = BulletGLMHelpers::convert(trans.getRotation());
        }
    }

    btDiscreteDynamicsWorld* world = physicsWorld.GetWorld();
    int numManifolds = world->getDispatcher()->getNumManifolds();

    std::set<CollisionPair> currentCollisions;

    for (int i = 0; i < numManifolds; i++)
    {
        btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
        const btCollisionObject* obA = contactManifold->getBody0();
        const btCollisionObject* obB = contactManifold->getBody1();

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

            if (scene.registry.valid(eA) && scene.registry.valid(eB))
            {
                if (eA > eB) std::swap(eA, eB);
                
                currentCollisions.insert({eA, eB});
                
                bool isTrigger = (obA->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) ||
                                 (obB->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE);

                bool isStay = m_activeCollisions.find({eA, eB}) != m_activeCollisions.end();

                auto Notify = [&](entt::entity target, entt::entity other, bool trigger, bool stay) {
                    if (scene.registry.all_of<ScriptComponent>(target))
                    {
                        auto& s = scene.registry.get<ScriptComponent>(target);
                        if (s.instance)
                        {
                            if (trigger)
                            {
                                if (!stay) s.instance->OnTriggerEnter(other);
                                else       s.instance->OnTriggerStay(other);
                            }
                            else
                            {
                                if (!stay) s.instance->OnCollisionEnter(other);
                                else       s.instance->OnCollisionStay(other);
                            }
                        }
                    }
                };

                Notify(eA, eB, isTrigger, isStay);
                Notify(eB, eA, isTrigger, isStay);
            }
        }
    }

    for (const auto& pair : m_activeCollisions)
    {
        if (currentCollisions.find(pair) == currentCollisions.end())
        {
            entt::entity eA = pair.first;
            entt::entity eB = pair.second;
            
            bool isTrigger = false; 
            if (scene.registry.all_of<RigidBodyComponent>(eA)) {
                if(auto* body = scene.registry.get<RigidBodyComponent>(eA).body)
                     if(body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) isTrigger = true;
            }
            if (!isTrigger && scene.registry.all_of<RigidBodyComponent>(eB)) {
                 if(auto* body = scene.registry.get<RigidBodyComponent>(eB).body)
                     if(body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) isTrigger = true;
            }

            auto NotifyExit = [&](entt::entity target, entt::entity other, bool trigger) {
                if (scene.registry.valid(target) && scene.registry.all_of<ScriptComponent>(target))
                {
                    auto& s = scene.registry.get<ScriptComponent>(target);
                    if (s.instance)
                    {
                        if (trigger) s.instance->OnTriggerExit(other);
                        else         s.instance->OnCollisionExit(other);
                    }
                }
            };

            NotifyExit(eA, eB, isTrigger);
            NotifyExit(eB, eA, isTrigger);
        }
    }

    m_activeCollisions = currentCollisions;
}
