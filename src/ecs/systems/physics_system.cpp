#include <ecs/systems/physics_system.h>
#include <physic/physics_transform_sync.h>
#include <physic/physics_collision_dispatcher.h>
#include <engine/ecs/cached_query.h>
#include <script/scriptable.h>
#include <physic/physic_world.h>
#include <utils/bullet_glm_helpers.h>
#include <glm/gtx/matrix_decompose.hpp>

PhysicsSystem::PhysicsSystem()
{
}

PhysicsSystem::~PhysicsSystem()
{
    WaitAsyncPhysics();
}

void PhysicsSystem::WaitAsyncPhysics()
{
    if (m_physicsFuture.valid())
    {
        m_physicsFuture.wait();
    }
}

void PhysicsSystem::Update(Scene &scene, PhysicsWorld &physicsWorld, float dt)
{
    if (!m_Enabled)
        return;

    if (!m_transformSync)
    {
        m_transformSync = std::make_unique<PhysicsTransformSync>(scene, physicsWorld);
        m_transformSync->Init();
    }

    m_transformSync->SyncToPhysics();

    if (m_AsyncPhysics)
    {
        WaitAsyncPhysics();

        m_physicsFuture = std::async(std::launch::async, [&physicsWorld, dt]()
                                     { physicsWorld.Update(dt); });
    }
    else
    {
        physicsWorld.Update(dt);
    }

    if (m_AsyncPhysics)
    {
        WaitAsyncPhysics();
    }

    m_transformSync->SyncFromPhysics();
    
    if (!m_collisionDispatcher)
    {
        m_collisionDispatcher = std::make_unique<PhysicsCollisionDispatcher>(scene, physicsWorld);
    }
    m_collisionDispatcher->DispatchEvents();
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
