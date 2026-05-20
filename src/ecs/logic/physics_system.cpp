#include <ecs/logic/physics_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>

// Forcing recompile - 2026-04-01-T11-55-00
#include <core/logic/config_manager.h>
#include <ecs/logic/cached_query.h>
#include <ecs/logic/system_factory.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <ecs/logic/entity_manager.h>
#include <physics/interface/i_collision_shape.h>
#include <physics/interface/i_physics_world.h>
#include <physics/logic/collision_matrix.h>
#include <physics/logic/physics_collision_dispatcher.h>
#include <physics/logic/physics_transform_sync.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <physics/unit/ray.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <render/interface/i_graphics_context.h>
#include <resource/logic/resource_manager.h>
#include <script/logic/scriptable.h>
#include <glm/gtx/matrix_decompose.hpp>

PhysicsSystem::PhysicsSystem()
{
}
PhysicsSystem::~PhysicsSystem()
{
}
REGISTER_SYSTEM(PhysicsSystem)

void PhysicsSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<PhysicsSystem>(this);
    auto* phys = sl.Resolve<IPhysicsWorld>();
    auto& configManager = sl.Require<ConfigManager>();

    if (phys)
    {
        const auto& cfg = configManager.GetConfig();
        phys->SetGravity(glm::vec3(cfg.gravity[0], cfg.gravity[1], cfg.gravity[2]));
        phys->SetMode(static_cast<int>(cfg.physicsMode));
        phys->SetSolverIterations(cfg.solverIterations);
        phys->SetCCDEnabled(cfg.ccdEnabled, cfg.ccdThreshold);
    }

    EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Physics | ConfigChangedEvent::All)))
            return;

        const auto& cfg = e.config;
        auto* phys_inner = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
        if (phys_inner)
        {
            phys_inner->SetGravity(glm::vec3(cfg.gravity[0], cfg.gravity[1], cfg.gravity[2]));
            phys_inner->SetMode(static_cast<int>(cfg.physicsMode));
            phys_inner->SetSolverIterations(cfg.solverIterations);
            phys_inner->SetCCDEnabled(cfg.ccdEnabled, cfg.ccdThreshold);
        }
    });

    EventManager::Instance().Subscribe<PhysicsDebugRenderEvent>([this](const PhysicsDebugRenderEvent& e) {
        if (!m_Enabled || !e.scene)
            return;
        auto& sl = ServiceLocator::Instance();
        if (auto* graphics = sl.Resolve<IGraphicsContext>())
        {
            if (auto* resources = sl.Resolve<ResourceManager>())
            {
                if (auto debugShader = resources->GetShader("debug_line"))
                {
                    RenderDebug(*e.scene, *debugShader, e.width, e.height, graphics->GetRenderStateManager());
                }
            }
        }
    });
}

void PhysicsSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    IPhysicsWorld* physicsWorld = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    if (!physicsWorld)
        return;

    if (&scene != m_LastScene || physicsWorld != m_LastPhysicsWorld)
    {
        Reset();
        m_LastScene = &scene;
        m_LastPhysicsWorld = physicsWorld;

        physicsWorld->SetCollisionFilter([pRegistry = &scene.registry](entt::entity eA, entt::entity eB) -> bool {
            if (!pRegistry || !pRegistry->valid(eA) || !pRegistry->valid(eB))
                return false;

            if (pRegistry->all_of<RigidBodyComponent>(eA) && !pRegistry->get<RigidBodyComponent>(eA).isCollisionEnabled)
                return false;
            if (pRegistry->all_of<RigidBodyComponent>(eB) && !pRegistry->get<RigidBodyComponent>(eB).isCollisionEnabled)
                return false;

            std::string tagA = "", nameA = "";
            std::string tagB = "", nameB = "";

            if (pRegistry->all_of<InfoComponent>(eA))
            {
                auto& info = pRegistry->get<InfoComponent>(eA);
                tagA = info.tag;
                nameA = info.name;
            }
            if (pRegistry->all_of<InfoComponent>(eB))
            {
                auto& info = pRegistry->get<InfoComponent>(eB);
                tagB = info.tag;
                nameB = info.name;
            }

            auto matrix = ServiceLocator::Instance().Resolve<CollisionMatrix>();
            if (matrix)
            {
                return matrix->CanCollide(tagA, tagB, nameA, nameB);
            }
            return true;
        });

        scene.registry.on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
        scene.registry.on_destroy<CharacterControllerComponent>()
            .connect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);

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
    for (auto entity : viewShape)
    {
        scene.registry.emplace<RigidBodyComponent>(entity);  // Default to static if only shape exists
    }

    auto viewInit = scene.registry.view<RigidShapeComponent, RigidBodyComponent>();
    for (auto entity : viewInit)
    {
        auto& rb = viewInit.get<RigidBodyComponent>(entity);
        if (!rb.body)
        {
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

    auto viewCC = scene.registry.view<CharacterControllerComponent, InfoComponent>();
    for (auto entity : viewCC)
    {
        auto& info = viewCC.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

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
        m_LastPhysicsWorld->SetCollisionFilter(nullptr);

    if (m_LastScene)
    {
        m_LastScene->registry.on_destroy<RigidBodyComponent>().disconnect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
        m_LastScene->registry.on_destroy<CharacterControllerComponent>()
            .disconnect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);
        m_LastScene->registry.on_construct<RigidShapeComponent>().disconnect<&PhysicsSystem::OnShapeConstructed>(this);
    }

    m_transformSync.reset();
    m_collisionDispatcher.reset();
    m_LastScene = nullptr;
    m_LastPhysicsWorld = nullptr;
}

void PhysicsSystem::OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity)
{
    if (!m_LastPhysicsWorld)
        return;
    auto& rb = registry.get<RigidBodyComponent>(entity);
    if (rb.body)
    {
        for (auto& constraint : rb.constraints)
        {
            if (constraint)
                m_LastPhysicsWorld->RemoveConstraint(constraint);
        }
        m_LastPhysicsWorld->RemoveRigidBody(rb.body.get());
    }
}

void PhysicsSystem::OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity)
{
    if (!m_LastPhysicsWorld)
        return;
    auto& cc = registry.get<CharacterControllerComponent>(entity);
    if (cc.controller)
    {
        m_LastPhysicsWorld->RemoveCharacterController(cc.controller.get());
    }
}

void PhysicsSystem::OnShapeConstructed(entt::registry& registry, entt::entity entity)
{
    // PhysicsSystem::Update will handle assembly
}

void PhysicsSystem::InitializeRigidBodyDirect(Scene& scene, entt::entity entity, RigidShapeComponent& shape,
                                              RigidBodyComponent& rb, IPhysicsWorld& physics)
{
    auto* pos = scene.registry.try_get<PositionComponent>(entity);
    auto* rot = scene.registry.try_get<RotationComponent>(entity);
    glm::vec3 worldPos = pos ? pos->value : glm::vec3(0, 0, 0);
    glm::quat worldRot = rot ? rot->value : glm::quat(1, 0, 0, 0);

    // Re-resolve world transform if possible
    if (auto* world = scene.registry.try_get<WorldTransformComponent>(entity))
    {
        glm::vec3 s, t, skew;
        glm::quat r;
        glm::vec4 perspective;
        if (glm::decompose(world->worldMatrix, s, r, t, skew, perspective))
        {
            worldPos = t;
            worldRot = r;
        }
    }

    std::shared_ptr<ICollisionShape> finalShape = nullptr;
    if (shape.type == ShapeType::Box)
        finalShape = physics.CreateBoxShape(shape.size);
    else if (shape.type == ShapeType::Sphere)
        finalShape = physics.CreateSphereShape(shape.radius);
    else if (shape.type == ShapeType::Capsule)
        finalShape = physics.CreateCapsuleShape(shape.radius, shape.height);
    else if (shape.type == ShapeType::Cylinder)
        finalShape = physics.CreateCylinderShape(shape.radius, shape.height);
    else if (shape.type == ShapeType::Mesh)
    {
        if (auto* meshComp = scene.registry.try_get<MeshRendererComponent>(entity))
        {
            if (meshComp->model)
            {
                std::vector<float> vertices;
                std::vector<uint32_t> indices;
                for (auto& mesh : meshComp->model->meshes)
                {
                    uint32_t offset = (uint32_t)vertices.size() / 3;
                    for (size_t vIdx = 0; vIdx < mesh.m_VertexCount; ++vIdx)
                    {
                        const float* pos =
                            reinterpret_cast<const float*>(mesh.m_VertexData.data() + vIdx * mesh.m_VertexStride);
                        vertices.push_back(pos[0]);
                        vertices.push_back(pos[1]);
                        vertices.push_back(pos[2]);
                    }
                    for (auto idx : mesh.indices)
                    {
                        indices.push_back(idx + offset);
                    }
                }
                if (!vertices.empty())
                    finalShape = physics.CreateMeshShape(vertices, indices);
            }
        }
    }
    else if (shape.type == ShapeType::Compound)
    {
        auto compound = physics.CreateCompoundShape();
        for (auto& cs : shape.children)
        {
            std::shared_ptr<ICollisionShape> child = nullptr;
            if (cs.type == ShapeType::Box)
                child = physics.CreateBoxShape(cs.size);
            else if (cs.type == ShapeType::Sphere)
                child = physics.CreateSphereShape(cs.radius);
            else if (cs.type == ShapeType::Capsule)
                child = physics.CreateCapsuleShape(cs.radius, cs.height);
            else if (cs.type == ShapeType::Cylinder)
                child = physics.CreateCylinderShape(cs.radius, cs.height);
            if (child)
                physics.AddChildShape(compound, child, cs.position, cs.rotation);
        }
        finalShape = compound;
    }

    bool hasOffset = glm::length(shape.offset) > 0.001f;
    bool hasRotation = std::abs(1.0f - std::abs(shape.rotation.w)) > 0.0001f;

    if (finalShape && (hasOffset || hasRotation))
    {
        auto compound = physics.CreateCompoundShape();
        physics.AddChildShape(compound, finalShape, shape.offset, shape.rotation);
        finalShape = compound;
    }

    if (finalShape)
    {
        // Fix scaling for Mesh shapes (apply asset-internal scale)
        if (shape.type == ShapeType::Mesh)
        {
            if (auto* meshComp = scene.registry.try_get<MeshRendererComponent>(entity))
            {
                if (meshComp->model)
                {
                    glm::mat4 rootMtx = meshComp->model->GetRootTransform();
                    glm::vec3 Rs, Rt, Rskew;
                    glm::quat Rr;
                    glm::vec4 Rperspective;
                    if (glm::decompose(rootMtx, Rs, Rr, Rt, Rskew, Rperspective))
                    {
                        auto* scl = scene.registry.try_get<ScaleComponent>(entity);
                        glm::vec3 totalScale = Rs * (scl ? scl->value : glm::vec3(1.0f));
                        finalShape->SetLocalScaling(totalScale);
                    }
                }
            }
        }

        float mass = rb.isStatic || rb.isKinematic ? 0.0f : rb.mass;
        rb.body = physics.CreateRigidBody(mass, worldPos, worldRot, finalShape);

        if (rb.body)
        {
            rb.body->SetUserPointer((void*)((uintptr_t)entity + 1));
            if (rb.isKinematic)
                rb.body->SetKinematic(true);

            rb.body->SetTrigger(rb.isTrigger);
            rb.body->SetFriction(shape.friction);
            rb.body->SetRestitution(shape.restitution);
            rb.body->SetLinearFactor(rb.linearFactor);
            rb.body->SetAngularFactor(rb.angularFactor);
            rb.body->SetDamping(rb.linearDamping, rb.angularDamping);
            physics.AddRigidBody(rb.body.get());
        }
    }
}

void PhysicsSystem::RenderDebug(Scene& scene, Shader& shader, int screenWidth, int screenHeight,
                                IRenderStateManager& renderState)
{
    if (!m_LastPhysicsWorld)
        return;

    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    auto& registry = scene.registry;

    if (!registry.valid(m_cachedPrimaryCamera) || !registry.all_of<CameraComponent>(m_cachedPrimaryCamera) ||
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
        auto& camera = registry.get<CameraComponent>(m_cachedPrimaryCamera);
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
    return {entt::type_id<InfoComponent>().hash(), entt::type_id<RigidBodyComponent>().hash(),
            entt::type_id<CharacterControllerComponent>().hash()};
}

std::vector<entt::id_type> PhysicsSystem::GetWriteComponents() const
{
    return {entt::type_id<PositionComponent>().hash(), entt::type_id<RotationComponent>().hash()};
}
