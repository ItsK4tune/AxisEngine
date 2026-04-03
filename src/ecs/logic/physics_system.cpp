#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/logic/physics_system.h>
// Forcing recompile - 2026-04-01-T11-55-00
#include <ecs/logic/system_factory.h>
#include <core/logic/config_manager.h>
#include <ecs/logic/cached_query.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <physics/interface/i_physics_world.h>
#include <physics/logic/collision_matrix.h>
#include <physics/logic/physics_collision_dispatcher.h>
#include <physics/logic/physics_transform_sync.h>
#include <script/logic/scriptable.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <core/logic/logger.h>
#include <physics/unit/ray.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <ecs/logic/entity_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <core/type/app_config.h>

PhysicsSystem::PhysicsSystem() {}
PhysicsSystem::~PhysicsSystem() {}
REGISTER_SYSTEM(PhysicsSystem)

void PhysicsSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<PhysicsSystem>(this);
    auto* phys = sl.Resolve<IPhysicsWorld>();
    auto& configManager = sl.Require<ConfigManager>();
    
    if (phys) {
        const auto& cfg = configManager.GetConfig();
        phys->SetGravity(glm::vec3(cfg.gravity[0], cfg.gravity[1], cfg.gravity[2]));
        phys->SetMode(static_cast<int>(cfg.physicsMode));
        phys->SetSolverIterations(cfg.solverIterations);
        phys->SetCCDEnabled(cfg.ccdEnabled, cfg.ccdThreshold);
    }

    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Physics | ConfigChangedEvent::All)))
            return;

        const auto& cfg = e.config;
        auto* phys_inner = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
        if (phys_inner) {
            phys_inner->SetGravity(glm::vec3(cfg.gravity[0], cfg.gravity[1], cfg.gravity[2]));
            phys_inner->SetMode(static_cast<int>(cfg.physicsMode));
            phys_inner->SetSolverIterations(cfg.solverIterations);
            phys_inner->SetCCDEnabled(cfg.ccdEnabled, cfg.ccdThreshold);
        }
    });
}

void PhysicsSystem::Update(Scene &scene, float dt)
{
    if (!m_Enabled) return;

    IPhysicsWorld* physicsWorld = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    if (!physicsWorld) return;

    if (&scene != m_LastScene || physicsWorld != m_LastPhysicsWorld)
    {
        Reset();
        m_LastScene = &scene;
        m_LastPhysicsWorld = physicsWorld;

        physicsWorld->SetCollisionFilter([pRegistry = &scene.registry](entt::entity eA, entt::entity eB) -> bool {
            if (!pRegistry || !pRegistry->valid(eA) || !pRegistry->valid(eB))
                return false;

            if (pRegistry->all_of<RigidBodyComponent>(eA) && !pRegistry->get<RigidBodyComponent>(eA).isCollisionEnabled) return false;
            if (pRegistry->all_of<RigidBodyComponent>(eB) && !pRegistry->get<RigidBodyComponent>(eB).isCollisionEnabled) return false;

            std::string tagA = "", nameA = "";
            std::string tagB = "", nameB = "";

            if (pRegistry->all_of<InfoComponent>(eA)) {
                auto& info = pRegistry->get<InfoComponent>(eA);
                tagA = info.tag; nameA = info.name;
            }
            if (pRegistry->all_of<InfoComponent>(eB)) {
                auto& info = pRegistry->get<InfoComponent>(eB);
                tagB = info.tag; nameB = info.name;
            }

            auto matrix = ServiceLocator::Instance().Resolve<CollisionMatrix>();
            return matrix ? matrix->CanCollide(tagA, tagB, nameA, nameB) : true;
        });

        scene.registry.on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
        scene.registry.on_destroy<CharacterControllerComponent>().connect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);
        
        // Listen for new shapes to initialize physics
        scene.registry.on_construct<RigidShapeComponent>().connect<&PhysicsSystem::OnShapeConstructed>(this);
    }
    
    if (!m_transformSync)
    {
        m_transformSync = std::make_unique<PhysicsTransformSync>(scene, *physicsWorld);
        m_transformSync->Initialize();
    }

    m_transformSync->SyncToPhysics();

    // Check for newly added components that need assembly
    auto viewShape = scene.registry.view<RigidShapeComponent>(entt::exclude<RigidBodyComponent>);
    for (auto entity : viewShape) {
        scene.registry.emplace<RigidBodyComponent>(entity); // Default to static if only shape exists
    }

    auto viewInit = scene.registry.view<RigidShapeComponent, RigidBodyComponent>();
    for (auto entity : viewInit) {
        auto& rb = viewInit.get<RigidBodyComponent>(entity);
        if (!rb.body) {
            InitializeRigidBodyDirect(scene, entity, viewInit.get<RigidShapeComponent>(entity), rb, *physicsWorld);
        }
    }

    physicsWorld->Update(dt);
    m_transformSync->SyncFromPhysics();

    if (!m_collisionDispatcher)
    {
        m_collisionDispatcher = std::make_unique<PhysicsCollisionDispatcher>(scene, *physicsWorld);
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
    if (m_LastPhysicsWorld) m_LastPhysicsWorld->SetCollisionFilter(nullptr);

    if (m_LastScene)
    {
        m_LastScene->registry.on_destroy<RigidBodyComponent>().disconnect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
        m_LastScene->registry.on_destroy<CharacterControllerComponent>().disconnect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);
        m_LastScene->registry.on_construct<RigidShapeComponent>().disconnect<&PhysicsSystem::OnShapeConstructed>(this);
    }

    m_transformSync.reset();
    m_collisionDispatcher.reset();
    m_LastScene = nullptr;
    m_LastPhysicsWorld = nullptr;
}

void PhysicsSystem::OnRigidBodyDestroyed(entt::registry &registry, entt::entity entity)
{
    if (!m_LastPhysicsWorld) return;
    auto &rb = registry.get<RigidBodyComponent>(entity);
    if (rb.body)
    {
        for (auto &constraint : rb.constraints)
        {
            if (constraint) m_LastPhysicsWorld->RemoveConstraint(constraint);
        }
        m_LastPhysicsWorld->RemoveRigidBody(rb.body.get());
    }
}

void PhysicsSystem::OnCharacterControllerDestroyed(entt::registry &registry, entt::entity entity)
{
    if (!m_LastPhysicsWorld) return;
    auto &cc = registry.get<CharacterControllerComponent>(entity);
    if (cc.controller)
    {
        m_LastPhysicsWorld->RemoveCharacterController(cc.controller.get());
    }
}

void PhysicsSystem::OnShapeConstructed(entt::registry& registry, entt::entity entity)
{
    // PhysicsSystem::Update will handle assembly
}

void PhysicsSystem::InitializeRigidBodyDirect(Scene& scene, entt::entity entity, RigidShapeComponent& shape, RigidBodyComponent& rb, IPhysicsWorld& physics)
{
    auto* pos = scene.registry.try_get<PositionComponent>(entity);
    auto* rot = scene.registry.try_get<RotationComponent>(entity);
    glm::vec3 worldPos = pos ? pos->value : glm::vec3(0,0,0);
    glm::quat worldRot = rot ? rot->value : glm::quat(1,0,0,0);

    // Re-resolve world transform if possible
    if (auto* world = scene.registry.try_get<WorldTransformComponent>(entity)) {
        glm::vec3 s, t, skew;
        glm::quat r;
        glm::vec4 perspective;
        if (glm::decompose(world->worldMatrix, s, r, t, skew, perspective)) {
            worldPos = t;
            worldRot = r;
        }
    }

    std::shared_ptr<ICollisionShape> finalShape = nullptr;
    if (shape.type == "BOX") finalShape = physics.CreateBoxShape(shape.size);
    else if (shape.type == "SPHERE") finalShape = physics.CreateSphereShape(shape.radius);
    else if (shape.type == "CAPSULE") finalShape = physics.CreateCapsuleShape(shape.radius, shape.height);
    else if (shape.type == "COMPOUND") {
        auto compound = physics.CreateCompoundShape();
        for (auto& cs : shape.children) {
            std::shared_ptr<ICollisionShape> child = nullptr;
            if (cs.type == "BOX") child = physics.CreateBoxShape(cs.size);
            else if (cs.type == "SPHERE") child = physics.CreateSphereShape(cs.radius);
            else if (cs.type == "CAPSULE") child = physics.CreateCapsuleShape(cs.radius, cs.height);
            if (child) physics.AddChildShape(compound, child, cs.position, cs.rotation);
        }
        finalShape = compound;
    }

    bool hasOffset = glm::length(shape.offset) > 0.001f;
    bool hasRotation = std::abs(1.0f - std::abs(shape.rotation.w)) > 0.0001f;
    
    if (finalShape && (hasOffset || hasRotation)) {
        auto compound = physics.CreateCompoundShape();
        physics.AddChildShape(compound, finalShape, shape.offset, shape.rotation);
        finalShape = compound;
    }

    if (finalShape) {
        float mass = rb.isStatic || rb.isKinematic ? 0.0f : rb.mass;
        rb.body = physics.CreateRigidBody(mass, worldPos, worldRot, finalShape);

        if (rb.body) {
            rb.body->SetUserPointer((void*)(uintptr_t)entity);
            if (rb.isKinematic) rb.body->SetKinematic(true);
            rb.body->SetFriction(shape.friction);
            rb.body->SetRestitution(shape.restitution);
            rb.body->SetLinearFactor(rb.linearFactor);
            rb.body->SetAngularFactor(rb.angularFactor);
            rb.body->SetDamping(rb.linearDamping, rb.angularDamping);
            physics.AddRigidBody(rb.body.get());
        }
    }
}

void PhysicsSystem::RenderDebug(Scene &scene, Shader &shader, int screenWidth, int screenHeight, IRenderStateManager &renderState)
{
    if (!m_LastPhysicsWorld) return;

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
            if (viewCamera.get<CameraComponent>(entity).isPrimary)
            {
                m_cachedPrimaryCamera = entity;
                break;
            }
        }
    }

    if (registry.valid(m_cachedPrimaryCamera))
    {
        auto &camera = registry.get<CameraComponent>(m_cachedPrimaryCamera);
        view = camera.viewMatrix;
        projection = camera.projectionMatrix;
    }

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    renderState.Disable(ServerCapability::DepthTest);
    m_LastPhysicsWorld->DebugDraw();
    renderState.Enable(ServerCapability::DepthTest);
}

std::vector<entt::id_type> PhysicsSystem::GetReadComponents() const
{
    return {
        entt::type_id<InfoComponent>().hash(),
        entt::type_id<RigidBodyComponent>().hash(),
        entt::type_id<CharacterControllerComponent>().hash()
    };
}

std::vector<entt::id_type> PhysicsSystem::GetWriteComponents() const
{
    return {
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash()
    };
}
