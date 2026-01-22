#include <ecs/systems/physics_system.h>
#include <engine/ecs/cached_query.h>
#include <script/scriptable.h>
#include <physic/physic_world.h>
#include <utils/bullet_glm_helpers.h>
#include <glm/gtx/matrix_decompose.hpp>

void PhysicsSystem::Update(Scene &scene, PhysicsWorld &physicsWorld, float dt)
{
    if (!m_Enabled)
        return;

    if (!m_signalsConnected)
    {
        m_connections.push_back(scene.registry.on_construct<RigidBodyComponent>().connect<&CachedQuery<RigidBodyComponent, TransformComponent>::MarkDirty>(m_simulationQuery));
        m_connections.push_back(scene.registry.on_destroy<RigidBodyComponent>().connect<&CachedQuery<RigidBodyComponent, TransformComponent>::MarkDirty>(m_simulationQuery));
        m_connections.push_back(scene.registry.on_construct<TransformComponent>().connect<&CachedQuery<RigidBodyComponent, TransformComponent>::MarkDirty>(m_simulationQuery));
        m_connections.push_back(scene.registry.on_destroy<TransformComponent>().connect<&CachedQuery<RigidBodyComponent, TransformComponent>::MarkDirty>(m_simulationQuery));
        m_signalsConnected = true;
    }

    m_simulationQuery.Update(scene.registry);
    m_worldMatrixCache.clear();

    physicsWorld.Update(dt);

    const auto &entities = m_simulationQuery.GetEntities();

    for (auto entity : entities)
    {
        auto &rb = scene.registry.get<RigidBodyComponent>(entity);
        auto &transform = scene.registry.get<TransformComponent>(entity);

        if (!rb.body)
            continue;

        bool isDynamic = !rb.body->isStaticOrKinematicObject();
        bool isKinematic = rb.body->isKinematicObject();
        bool hasParent = scene.registry.valid(transform.parent);

        if (isDynamic && rb.body->isActive())
        {
            btTransform trans;
            if (rb.body->getMotionState())
                rb.body->getMotionState()->getWorldTransform(trans);
            else
                trans = rb.body->getWorldTransform();

            glm::vec3 worldPos = BulletGLMHelpers::convert(trans.getOrigin());
            glm::quat worldRot = BulletGLMHelpers::convert(trans.getRotation());

            if (hasParent && rb.isParentMatter)
            {
                if (scene.registry.all_of<TransformComponent>(transform.parent))
                {
                    glm::mat4 parentWorldMatrix = GetCachedWorldMatrix(transform.parent, scene.registry);
                    glm::mat4 validWorldMatrix = glm::translate(glm::mat4(1.0f), worldPos) * glm::mat4_cast(worldRot);

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
            else
            {
                transform.position = worldPos;
                transform.rotation = worldRot;
            }
        }

        if ((!isDynamic || isKinematic) && hasParent)
        {
            glm::mat4 worldMatrix = GetCachedWorldMatrix(entity, scene.registry);
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

    btDiscreteDynamicsWorld *world = physicsWorld.GetWorld();
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

            if (scene.registry.valid(eA) && scene.registry.valid(eB))
            {
                if (eA > eB)
                    std::swap(eA, eB);

                currentCollisions.insert({eA, eB});

                bool isTrigger = (obA->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) ||
                                 (obB->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE);

                bool isStay = m_activeCollisions.count({eA, eB}) > 0;

                auto Notify = [&](entt::entity target, entt::entity other, bool trigger, bool stay)
                {
                    if (scene.registry.all_of<ScriptComponent>(target))
                    {
                        auto &s = scene.registry.get<ScriptComponent>(target);
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
            entt::entity eA = pair.first;
            entt::entity eB = pair.second;

            bool isTrigger = false;
            if (scene.registry.all_of<RigidBodyComponent>(eA))
            {
                if (auto *body = scene.registry.get<RigidBodyComponent>(eA).body)
                    if (body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
                        isTrigger = true;
            }
            if (!isTrigger && scene.registry.all_of<RigidBodyComponent>(eB))
            {
                if (auto *body = scene.registry.get<RigidBodyComponent>(eB).body)
                    if (body->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
                        isTrigger = true;
            }

            auto NotifyExit = [&](entt::entity target, entt::entity other, bool trigger)
            {
                if (scene.registry.valid(target) && scene.registry.all_of<ScriptComponent>(target))
                {
                    auto &s = scene.registry.get<ScriptComponent>(target);
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

void PhysicsSystem::RenderDebug(Scene &scene, PhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight)
{
    DebugDrawer *drawer = physicsWorld.GetDebugDrawer();
    if (!drawer)
        return;

    physicsWorld.GetWorld()->debugDrawWorld();

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    auto &registry = scene.registry;
    
    if (!registry.valid(m_cachedPrimaryCamera) || 
        !registry.all_of<CameraComponent>(m_cachedPrimaryCamera) ||
        !registry.get<CameraComponent>(m_cachedPrimaryCamera).isPrimary)
    {
        m_cachedPrimaryCamera = entt::null;
        auto viewCamera = registry.view<CameraComponent, TransformComponent>();
        for (auto entity : viewCamera)
        {
            auto &camera = viewCamera.get<CameraComponent>(entity);
            if (camera.isPrimary)
            {
                m_cachedPrimaryCamera = entity;
                break;
            }
        }
    }

    if (registry.valid(m_cachedPrimaryCamera))
    {
        auto &camera = registry.get<CameraComponent>(m_cachedPrimaryCamera);
        auto &transform = registry.get<TransformComponent>(m_cachedPrimaryCamera);

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
    }

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    glDisable(GL_DEPTH_TEST);
    drawer->Flush();
    glEnable(GL_DEPTH_TEST);

    drawer->FrameStart();
}

glm::mat4 PhysicsSystem::GetCachedWorldMatrix(entt::entity entity, entt::registry &registry)
{
    auto it = m_worldMatrixCache.find(entity);
    if (it != m_worldMatrixCache.end())
        return it->second;

    glm::mat4 result;
    if (registry.all_of<TransformComponent>(entity))
    {
        const auto &tc = registry.get<TransformComponent>(entity);
        
        glm::mat4 local = tc.GetLocalModelMatrix();
        
        if (registry.valid(tc.parent))
        {
            glm::mat4 parentWorld = GetCachedWorldMatrix(tc.parent, registry);
            result = parentWorld * local;
        }
        else
        {
            result = local;
        }
    }
    else
    {
        result = glm::mat4(1.0f);
    }

    // Cache it
    m_worldMatrixCache[entity] = result;
    return result;
}
