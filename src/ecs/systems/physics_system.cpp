#include <ecs/systems/physics_system.h>
#include <utils/logger.h>
#include <physic/physics_transform_sync.h>
#include <physic/physics_collision_dispatcher.h>
#include <engine/ecs/cached_query.h>
#include <script/scriptable.h>

#include <interface/physics/i_physics_world.h>
#include <utils/bullet_glm_helpers.h>
#include <glm/gtx/matrix_decompose.hpp>

PhysicsSystem::PhysicsSystem()
{
}

PhysicsSystem::~PhysicsSystem()
{
}

void PhysicsSystem::Update(Scene &scene, IPhysicsWorld &physicsWorld, float dt)
{
    if (!m_Enabled)
        return;

    if (&scene != m_LastScene || &physicsWorld != m_LastPhysicsWorld)
    {
        LOGGER_INFO("PhysicsSystem") << "Scene or PhysicsWorld changed reinitializing subsystems";
        Reset();
        m_LastScene = &scene;
        m_LastPhysicsWorld = &physicsWorld;
    }

    if (!m_transformSync)
    {
        LOGGER_INFO("PhysicsSystem") << "Initializing Physics Transform Sync";
        m_transformSync = std::make_unique<PhysicsTransformSync>(scene, physicsWorld);
        m_transformSync->Init();
    }

    m_transformSync->SyncToPhysics();

    physicsWorld.Update(dt);

    m_transformSync->SyncFromPhysics();

    if (!m_collisionDispatcher)
    {
        LOGGER_INFO("PhysicsSystem") << "Initializing Physics Collision Dispatcher";
        m_collisionDispatcher = std::make_unique<PhysicsCollisionDispatcher>(scene, physicsWorld);
    }
    m_collisionDispatcher->DispatchEvents();
}

void PhysicsSystem::Reset()
{
    m_transformSync.reset();
    m_collisionDispatcher.reset();
    m_LastScene = nullptr;
    m_LastPhysicsWorld = nullptr;
}

void PhysicsSystem::RenderDebug(Scene &scene, IPhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight, IRenderStateManager &renderState)
{
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

    renderState.Disable(Graphics::ServerCapability::DepthTest);
    physicsWorld.DebugDraw(); 
    renderState.Enable(Graphics::ServerCapability::DepthTest);
}
