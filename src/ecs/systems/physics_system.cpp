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
