#include <ecs/components/info_component.h>
#include <ecs/systems/physics_system.h>
#include <engine/ecs/cached_query.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <systems/physics/interfaces/i_physics_world.h>
#include <systems/physics/collision_matrix.h>
#include <systems/physics/physics_collision_dispatcher.h>
#include <systems/physics/physics_transform_sync.h>
#include <core/scripting/scriptable.h>
#include <core/utils/bullet_glm_helpers.h>
#include <core/utils/logger.h>
#include <systems/physics/raycast.h>
#include <systems/window/io_handler.h>
#include <systems/window/monitor_manager.h>
#include <ecs/components/camera_component.h>
#include <ecs/entity_manager.h>

PhysicsSystem::PhysicsSystem()
{
}

PhysicsSystem::~PhysicsSystem()
{
}

void PhysicsSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled)
        return;

    if (&scene != m_LastScene || m_Ctx.physics != m_LastPhysicsWorld)
    {
        LOGGER_INFO("PhysicsSystem") << "Scene or PhysicsWorld changed reinitializing subsystems";
        Reset();
        m_LastScene = &scene;
        m_LastPhysicsWorld = m_Ctx.physics;

        m_Ctx.physics->SetCollisionFilter([&scene](entt::entity eA, entt::entity eB) -> bool {
            if (!scene.registry.valid(eA) || !scene.registry.valid(eB))
                return false;

            if (scene.registry.all_of<RigidBodyComponent>(eA)) {
                if (!scene.registry.get<RigidBodyComponent>(eA).isCollisionEnabled)
                    return false;
            }

            if (scene.registry.all_of<RigidBodyComponent>(eB)) {
                if (!scene.registry.get<RigidBodyComponent>(eB).isCollisionEnabled)
                    return false;
            }

            std::string tagA = "", nameA = "";
            std::string tagB = "", nameB = "";

            if (scene.registry.all_of<InfoComponent>(eA)) {
                auto& info = scene.registry.get<InfoComponent>(eA);
                tagA = info.tag;
                nameA = info.name;
            }
            
            if (scene.registry.all_of<InfoComponent>(eB)) {
                auto& info = scene.registry.get<InfoComponent>(eB);
                tagB = info.tag;
                nameB = info.name;
            }

            return CollisionMatrix::Instance().CanCollide(tagA, tagB, nameA, nameB);
        });

        scene.registry.on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
        scene.registry.on_destroy<CharacterControllerComponent>().connect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);
    }
    if (!m_transformSync)
    {
        LOGGER_INFO("PhysicsSystem") << "Initializing Physics Transform Sync";
        m_transformSync = std::make_unique<PhysicsTransformSync>(scene, *m_Ctx.physics);
        m_transformSync->Init();
    }

    m_transformSync->SyncToPhysics();

    m_Ctx.physics->Update(dt);

    m_transformSync->SyncFromPhysics();

    if (!m_collisionDispatcher)
    {
        LOGGER_INFO("PhysicsSystem") << "Initializing Physics Collision Dispatcher";
        m_collisionDispatcher = std::make_unique<PhysicsCollisionDispatcher>(scene, *m_Ctx.physics);
    }
    
    m_collisionDispatcher->DispatchEvents();

    auto viewCC = scene.registry.view<CharacterControllerComponent>();
    for (auto entity : viewCC)
    {
        auto& cc = viewCC.get<CharacterControllerComponent>(entity);
        if (cc.controller)
        {
            cc.controller->SetWalkDirection(cc.walkDirection);
            if (cc.jumpRequested)
            {
                cc.controller->Jump();
                cc.jumpRequested = false;
            }
            cc.isOnGround = cc.controller->OnGround();
        }
    }
}

void PhysicsSystem::Reset()
{
    if (m_LastPhysicsWorld)
    {
        m_LastPhysicsWorld->SetCollisionFilter(nullptr);
    }

    if (m_LastScene)
    {
        m_LastScene->registry.on_destroy<RigidBodyComponent>().disconnect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
        m_LastScene->registry.on_destroy<CharacterControllerComponent>().disconnect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);
    }

    m_transformSync.reset();
    m_collisionDispatcher.reset();
    m_LastScene = nullptr;
    m_LastPhysicsWorld = nullptr;
}

void PhysicsSystem::OnRigidBodyDestroyed(entt::registry &registry, entt::entity entity)
{
    if (!m_LastPhysicsWorld)
        return;

    if (registry.all_of<RigidBodyComponent>(entity))
    {
        auto &rb = registry.get<RigidBodyComponent>(entity);
        if (rb.body)
        {
            try {
                for (auto &constraint : rb.constraints)
                {
                    if (constraint)
                        m_LastPhysicsWorld->RemoveConstraint(constraint);
                }
                
                m_LastPhysicsWorld->RemoveRigidBody(rb.body.get());
            } catch (...) {
                LOGGER_ERROR("PhysicsSystem") << "OnRigidBodyDestroyed: CRASH during rigid body cleanup for entity " << (uint32_t)entity;
            }
        }
    }
}

void PhysicsSystem::OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity)
{
    if (!m_LastPhysicsWorld)
        return;

    if (registry.all_of<CharacterControllerComponent>(entity))
    {
        auto& cc = registry.get<CharacterControllerComponent>(entity);
        if (cc.controller)
        {
            m_LastPhysicsWorld->RemoveCharacterController(cc.controller.get());
        }
    }
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
        auto viewCamera = registry.view<CameraComponent, PositionComponent>();
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
        auto* posComp = registry.try_get<PositionComponent>(m_cachedPrimaryCamera);
        glm::vec3 pos = posComp ? posComp->value : glm::vec3(0.0f);

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
    m_Ctx.physics->DebugDraw();
    renderState.Enable(Graphics::ServerCapability::DepthTest);
}

RayHit PhysicsSystem::Raycast(const glm::vec3 &origin, const glm::vec3 &direction, float distance)
{
    if (!m_Ctx.physics)
        return {};
    return m_Ctx.physics->Raycast(origin, direction, distance);
}

RayHit PhysicsSystem::Raycast(const glm::vec3 &start, const glm::vec3 &end)
{
    if (!m_Ctx.physics)
        return {};

    glm::vec3 dir = end - start;
    float dist = glm::length(dir);
    if (dist < 0.0001f)
        return {};

    return m_Ctx.physics->Raycast(start, glm::normalize(dir), dist);
}

RayHit PhysicsSystem::Raycast(const glm::vec3 &origin, float yaw, float pitch, float distance)
{
    if (!m_Ctx.physics)
        return {};

    glm::vec3 dir = RaycastUtils::AngleToDirection(yaw, pitch);
    return m_Ctx.physics->Raycast(origin, dir, distance);
}

RayHit PhysicsSystem::RaycastFromScreen(const glm::vec2 &screenPos, float distance)
{
    if (!m_Ctx.physics || !m_LastScene)
        return {};

    entt::entity camEntity = EntityManager::GetActiveCamera(*m_LastScene);
    if (camEntity == entt::null)
        return {};

    auto &camera = m_LastScene->registry.get<CameraComponent>(camEntity);
    glm::vec2 viewportSize(m_Ctx.io->GetMonitorManager().GetWidth(), m_Ctx.io->GetMonitorManager().GetHeight());

    RaycastUtils::Ray ray = RaycastUtils::CalculateRay(screenPos, viewportSize, camera.viewMatrix, camera.projectionMatrix);

    return m_Ctx.physics->Raycast(ray.origin, ray.direction, distance);
}

