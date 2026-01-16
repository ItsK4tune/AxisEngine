#include <ecs/system.h>
#include <core/scriptable.h>
#include <physic/physic_world.h>
#include <iostream>
#include <glm/gtx/matrix_decompose.hpp>

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

            if (scene.registry.valid(transform.parent)) {
                 if(scene.registry.all_of<TransformComponent>(transform.parent)) {
                     const auto& parentTrans = scene.registry.get<TransformComponent>(transform.parent);
                     glm::mat4 parentWorldMatrix = parentTrans.GetWorldModelMatrix(scene.registry);
                     glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4_cast(transform.rotation); 
                     
                     glm::mat4 localMatrix = glm::inverse(parentWorldMatrix) * validWorldMatrix;

                     glm::vec3 s;
                     glm::quat r;
                     glm::vec3 t;
                     glm::vec3 skew;
                     glm::vec4 perspective;
                     glm::decompose(localMatrix, s, r, t, skew, perspective);
                     
                     transform.position = t;
                     transform.rotation = r;
                 }
            }
        }
    }

    for (auto entity : view)
    {
        auto &rb = view.get<RigidBodyComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);

        if (rb.body && scene.registry.valid(transform.parent))
        {
            bool isDynamic = !rb.body->isStaticOrKinematicObject();
            
            if (!isDynamic || rb.body->isKinematicObject())
            {
                 glm::mat4 worldMatrix = transform.GetWorldModelMatrix(scene.registry);
                 glm::vec3 worldPos = glm::vec3(worldMatrix[3]);
                 glm::quat worldRot = glm::quat_cast(worldMatrix);

                 btTransform tr;
                 tr.setIdentity();
                 tr.setOrigin(BulletGLMHelpers::convert(worldPos));
                 tr.setRotation(BulletGLMHelpers::convert(worldRot));

                 rb.body->setWorldTransform(tr);
                 if (rb.body->getMotionState())
                 {
                     rb.body->getMotionState()->setWorldTransform(tr);
                 }
            }
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

void PhysicsSystem::RenderDebug(Scene &scene, PhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight)
{
    DebugDrawer* drawer = physicsWorld.GetDebugDrawer();
    if (!drawer) return;

    // Draw the physics world (populates the drawer)
    physicsWorld.GetWorld()->debugDrawWorld();

    // Setup Camera
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    auto viewCamera = scene.registry.view<CameraComponent, TransformComponent>();
    for (auto entity : viewCamera)
    {
        auto &camera = viewCamera.get<CameraComponent>(entity);
        if (camera.isPrimary)
        {
            auto &transform = viewCamera.get<TransformComponent>(entity);
            
            // Recompute View/Proj (Code duplication from RenderSystem, but acceptable for now or refactor helper)
            glm::vec3 pos = transform.position;
            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front = glm::normalize(front);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

            view = glm::lookAt(pos, pos + front, up);
            
            float aspect = (float)screenWidth / (float)screenHeight;
            
            projection = glm::perspective(glm::radians(camera.fov), aspect, camera.nearPlane, camera.farPlane);
            break;
        }
    }

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glDisable(GL_DEPTH_TEST);
    drawer->Flush();
    glEnable(GL_DEPTH_TEST);
    
    // Clear lines for next frame
    drawer->FrameStart(); 
}
