#include <ecs/logic/physics_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>

#include <core/logic/config_manager.h>
#include <ecs/logic/cached_query.h>
#include <ecs/logic/system_factory.h>

#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <physics/interface/i_collision_shape.h>
#include <physics/interface/i_physics_world.h>
#include <physics/logic/collision_matrix.h>
#include <physics/logic/physics_collision_dispatcher.h>
#include <physics/logic/constraint_lifecycle.h>
#include <physics/logic/physics_transform_sync.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <physics/unit/ray.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <render/interface/i_graphics_context.h>
#include <resource/logic/resource_manager.h>
#include <script/logic/scriptable.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <functional>

PhysicsSystem::PhysicsSystem()
{
}
PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

void PhysicsSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<PhysicsSystem>(this);
    m_EventSubscriptions.Clear();
    auto* phys = sl.Resolve<IPhysicsWorld>();
    auto& configManager = sl.Require<ConfigManager>();

    if (phys)
    {
        auto cfg = configManager.GetConfig();
        phys->SetGravity(glm::vec3(cfg.physics.gravity[0], cfg.physics.gravity[1], cfg.physics.gravity[2]));
        phys->SetMode(static_cast<int>(cfg.physics.physicsMode));
        phys->SetSimulationSettings(
            cfg.physics.physicsTickRate > 0.0f ? 1.0f / cfg.physics.physicsTickRate : 1.0f / 60.0f,
            cfg.physics.maxSubSteps);
        phys->SetSolverIterations(cfg.physics.solverIterations);
        phys->SetCCDEnabled(cfg.physics.ccdEnabled, cfg.physics.ccdThreshold);
    }

    m_EventSubscriptions.Add(EventManager::Instance().Subscribe<ConfigChangedEvent>([this](
                                                                                        const ConfigChangedEvent& e) {
        if (!HasConfigChanged(e, ConfigChangedEvent::Physics))
            return;

        const auto& cfg = e.config;
        auto* phys_inner = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
        if (phys_inner)
        {
            phys_inner->SetGravity(glm::vec3(cfg.physics.gravity[0], cfg.physics.gravity[1], cfg.physics.gravity[2]));
            phys_inner->SetMode(static_cast<int>(cfg.physics.physicsMode));
            phys_inner->SetSimulationSettings(
                cfg.physics.physicsTickRate > 0.0f ? 1.0f / cfg.physics.physicsTickRate : 1.0f / 60.0f,
                cfg.physics.maxSubSteps);
            phys_inner->SetSolverIterations(cfg.physics.solverIterations);
            phys_inner->SetCCDEnabled(cfg.physics.ccdEnabled, cfg.physics.ccdThreshold);
        }
    }));

    m_EventSubscriptions.Add(
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
        }));
}

void PhysicsSystem::Shutdown()
{
    m_EventSubscriptions.Clear();
    Reset();
}

void PhysicsSystem::Update(Scene& scene, float dt)
{
    if (dt <= 0.0f)
        Step(scene, dt);
}

void PhysicsSystem::FixedUpdate(Scene& scene, float fixedDt)
{
    Step(scene, fixedDt);
}

void PhysicsSystem::Step(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    auto& sl = ServiceLocator::Instance();
    IPhysicsWorld* physicsWorld = sl.Resolve<IPhysicsWorld>();
    if (!physicsWorld)
        return;

    if (&scene != m_LastScene || physicsWorld != m_LastPhysicsWorld)
    {
        Reset();
        m_LastScene = &scene;
        m_LastPhysicsWorld = physicsWorld;

        auto* collisionMatrix = sl.Resolve<CollisionMatrix>();
        physicsWorld->SetCollisionFilter(
            [pRegistry = &scene.GetRegistry(), collisionMatrix](entt::entity eA, entt::entity eB) -> bool {
                if (!pRegistry || !pRegistry->valid(eA) || !pRegistry->valid(eB))
                    return false;

                if (auto* rbA = pRegistry->try_get<RigidBodyComponent>(eA); rbA && !rbA->isCollisionEnabled)
                    return false;
                if (auto* rbB = pRegistry->try_get<RigidBodyComponent>(eB); rbB && !rbB->isCollisionEnabled)
                    return false;

                if (!collisionMatrix || collisionMatrix->IsEmpty())
                    return true;

                static const std::string emptyString;
                const auto* infoA = pRegistry->try_get<InfoComponent>(eA);
                const auto* infoB = pRegistry->try_get<InfoComponent>(eB);
                const std::string& tagA = infoA ? infoA->tag : emptyString;
                const std::string& tagB = infoB ? infoB->tag : emptyString;
                const std::string& nameA = infoA ? infoA->name : emptyString;
                const std::string& nameB = infoB ? infoB->name : emptyString;

                return collisionMatrix->CanCollide(tagA, tagB, nameA, nameB);
            });

        scene.GetRegistry().on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
        scene.GetRegistry()
            .on_destroy<CharacterControllerComponent>()
            .connect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);

        scene.GetRegistry().on_construct<RigidShapeComponent>().connect<&PhysicsSystem::OnShapeConstructed>(this);
        scene.GetRegistry().on_update<RigidShapeComponent>().connect<&PhysicsSystem::OnShapeUpdated>(this);
        scene.GetRegistry().on_destroy<RigidShapeComponent>().connect<&PhysicsSystem::OnShapeDestroyed>(this);

        auto shapeView = scene.GetRegistry().view<RigidShapeComponent>();
        m_PendingRigidBodies.reserve(shapeView.size());
        for (auto entity : shapeView)
        {
            if (!scene.GetRegistry().all_of<RigidBodyComponent>(entity))
                scene.GetRegistry().emplace<RigidBodyComponent>(entity);
            m_PendingRigidBodies.insert(entity);
        }
    }

    if (!m_transformSync)
    {
        m_transformSync = std::make_unique<PhysicsTransformSync>(scene, *physicsWorld);
        m_transformSync->Initialize();
    }

    m_transformSync->SyncToPhysics();

    for (auto it = m_PendingRigidBodies.begin(); it != m_PendingRigidBodies.end();)
    {
        const entt::entity entity = *it;
        if (!scene.IsValid(entity) ||
            !scene.GetRegistry().all_of<RigidShapeComponent, RigidBodyComponent>(entity))
        {
            it = m_PendingRigidBodies.erase(it);
            continue;
        }

        auto& rb = scene.GetRegistry().get<RigidBodyComponent>(entity);
        if (!rb.body)
            InitializeRigidBodyDirect(scene, entity, scene.GetRegistry().get<RigidShapeComponent>(entity), rb,
                                      *physicsWorld);
        if (rb.body)
            it = m_PendingRigidBodies.erase(it);
        else
            ++it;
    }

    m_ControllerQuery.Update(scene.GetRegistry());
    for (auto entity : m_ControllerQuery.GetEntities())
    {
        auto& info = scene.GetRegistry().get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto& cc = scene.GetRegistry().get<CharacterControllerComponent>(entity);
        if (!cc.controller)
            continue;
        if (cc.useVelocity)
        {
            if (glm::length(cc.velocity) > 0.0001f)
                cc.controller->SetVelocity(cc.velocity, dt);
            else
                cc.controller->SetWalkDirection(glm::vec3(0.0f));
        }
        else
        {
            cc.controller->SetWalkDirection(cc.walkDirection);
        }
        if (cc.jumpRequested)
        {
            cc.controller->Jump();
            cc.jumpRequested = false;
        }
    }

    physicsWorld->Update(dt);
    m_transformSync->SyncFromPhysics();

    if (!m_collisionDispatcher)
    {
        m_collisionDispatcher = std::make_unique<PhysicsCollisionDispatcher>(scene, *physicsWorld);
    }
    m_collisionDispatcher->DispatchEvents();

    for (auto entity : m_ControllerQuery.GetEntities())
    {
        if (!scene.IsValid(entity) ||
            !scene.GetRegistry().all_of<CharacterControllerComponent, InfoComponent>(entity))
            continue;
        auto& cc = scene.GetRegistry().get<CharacterControllerComponent>(entity);
        if (!scene.GetRegistry().get<InfoComponent>(entity).isActive)
            continue;
        if (cc.controller)
        {
            cc.isOnGround = cc.controller->OnGround();
            if (auto* pos = scene.GetRegistry().try_get<PositionComponent>(entity))
            {
                glm::vec3 controllerPos;
                glm::quat controllerRot;
                cc.controller->GetWorldTransform(controllerPos, controllerRot);
                pos->value = controllerPos;
                if (auto* rot = scene.GetRegistry().try_get<RotationComponent>(entity))
                    rot->value = controllerRot;
                scene.MarkTransformDirty(entity);
            }
        }
    }
}

void PhysicsSystem::Reset()
{
    if (m_LastPhysicsWorld)
        m_LastPhysicsWorld->SetCollisionFilter(nullptr);

    if (m_LastScene)
    {
        m_LastScene->GetRegistry().on_destroy<RigidBodyComponent>().disconnect<&PhysicsSystem::OnRigidBodyDestroyed>(
            this);
        m_LastScene->GetRegistry()
            .on_destroy<CharacterControllerComponent>()
            .disconnect<&PhysicsSystem::OnCharacterControllerDestroyed>(this);
        m_LastScene->GetRegistry().on_construct<RigidShapeComponent>().disconnect<&PhysicsSystem::OnShapeConstructed>(
            this);
        m_LastScene->GetRegistry().on_update<RigidShapeComponent>().disconnect<&PhysicsSystem::OnShapeUpdated>(this);
        m_LastScene->GetRegistry().on_destroy<RigidShapeComponent>().disconnect<&PhysicsSystem::OnShapeDestroyed>(this);
    }

    m_transformSync.reset();
    m_collisionDispatcher.reset();
    m_LastScene = nullptr;
    m_LastPhysicsWorld = nullptr;
    m_PendingRigidBodies.clear();
    m_ControllerQuery.Clear();
    m_MeshShapeCache.clear();
}

void PhysicsSystem::OnRigidBodyDestroyed(entt::registry& registry, entt::entity entity)
{
    m_PendingRigidBodies.erase(entity);
    if (!m_LastPhysicsWorld)
        return;
    auto& rb = registry.get<RigidBodyComponent>(entity);
    PhysicsConstraintLifecycle::RemoveAll(registry, *m_LastPhysicsWorld, rb);
    if (rb.body)
    {
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
    if (!registry.all_of<RigidBodyComponent>(entity))
        registry.emplace<RigidBodyComponent>(entity);
    m_PendingRigidBodies.insert(entity);
}

void PhysicsSystem::OnShapeDestroyed(entt::registry& registry, entt::entity entity)
{
    m_PendingRigidBodies.erase(entity);
    if (!m_LastPhysicsWorld)
        return;
    if (auto* rb = registry.try_get<RigidBodyComponent>(entity); rb && rb->body)
    {
        PhysicsConstraintLifecycle::RemoveAll(registry, *m_LastPhysicsWorld, *rb);
        m_LastPhysicsWorld->RemoveRigidBody(rb->body.get());
        rb->body.reset();
    }
}

void PhysicsSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    m_MeshShapeCacheEnabled = config.physicsMeshShapeCacheEnabled;
    if (!m_MeshShapeCacheEnabled)
        m_MeshShapeCache.clear();
}

void PhysicsSystem::OnShapeUpdated(entt::registry& registry, entt::entity entity)
{
    if (!registry.all_of<RigidBodyComponent>(entity))
    {
        registry.emplace<RigidBodyComponent>(entity);
    }
    else if (m_LastPhysicsWorld)
    {
        auto& rb = registry.get<RigidBodyComponent>(entity);
        if (rb.body)
        {
            PhysicsConstraintLifecycle::RemoveAll(registry, *m_LastPhysicsWorld, rb);
            m_LastPhysicsWorld->RemoveRigidBody(rb.body.get());
            rb.body.reset();
        }
    }
    m_PendingRigidBodies.insert(entity);
}

void PhysicsSystem::InitializeRigidBodyDirect(Scene& scene, entt::entity entity, RigidShapeComponent& shape,
                                              RigidBodyComponent& rb, IPhysicsWorld& physics)
{
    auto* pos = scene.GetRegistry().try_get<PositionComponent>(entity);
    auto* rot = scene.GetRegistry().try_get<RotationComponent>(entity);
    glm::vec3 worldPos = pos ? pos->value : glm::vec3(0, 0, 0);
    glm::quat worldRot = rot ? rot->value : glm::quat(1, 0, 0, 0);

    if (auto* world = scene.GetRegistry().try_get<WorldTransformComponent>(entity))
    {
        if (world->version > 0)
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
    }

    const glm::vec3 entityScale = scene.GetRegistry().all_of<ScaleComponent>(entity)
                                      ? scene.GetRegistry().get<ScaleComponent>(entity).value
                                      : glm::vec3(1.0f);

    auto createMeshShape = [&](const glm::vec3& vertexScale) -> std::shared_ptr<ICollisionShape> {
        auto* meshComp = scene.GetRegistry().try_get<MeshRendererComponent>(entity);
        if (!meshComp || !meshComp->model)
            return nullptr;

        const MeshShapeCacheKey cacheKey{meshComp->model.get(), vertexScale};
        if (m_MeshShapeCacheEnabled)
            if (const auto cached = m_MeshShapeCache.find(cacheKey); cached != m_MeshShapeCache.end())
                if (auto cachedShape = cached->second.lock())
                    return cachedShape;

        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        for (auto& mesh : meshComp->model->meshes)
        {
            const uint32_t offset = static_cast<uint32_t>(vertices.size() / 3);
            for (size_t vertexIndex = 0; vertexIndex < mesh.m_VertexCount; ++vertexIndex)
            {
                const glm::vec3 vertex = mesh.GetPosition(vertexIndex) * vertexScale;
                vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            }
            for (auto index : mesh.indices)
                indices.push_back(index + offset);
        }
        auto result = physics.CreateMeshShape(vertices, indices);
        if (result && m_MeshShapeCacheEnabled)
            m_MeshShapeCache[cacheKey] = result;
        return result;
    };

    std::function<std::shared_ptr<ICollisionShape>(const RigidShapeComponent::ChildShape&)> createChildShape;
    createChildShape = [&](const RigidShapeComponent::ChildShape& childData) -> std::shared_ptr<ICollisionShape> {
        std::shared_ptr<ICollisionShape> child;
        switch (childData.type)
        {
            case ShapeType::Box: child = physics.CreateBoxShape(childData.size); break;
            case ShapeType::Sphere: child = physics.CreateSphereShape(childData.radius); break;
            case ShapeType::Capsule: child = physics.CreateCapsuleShape(childData.radius, childData.height); break;
            case ShapeType::Cylinder: child = physics.CreateCylinderShape(childData.radius, childData.height); break;
            case ShapeType::Mesh: child = createMeshShape(glm::vec3(1.0f)); break;
            case ShapeType::Heightfield:
                if (IsValidHeightfieldSize(childData.heightfieldWidth, childData.heightfieldLength,
                                           childData.heightSamples.size()))
                {
                    child = physics.CreateHeightfieldShape(childData.heightSamples, childData.heightfieldWidth,
                                                           childData.heightfieldLength, childData.minHeight,
                                                           childData.maxHeight);
                    if (child)
                        child->SetLocalScaling(childData.heightfieldScale);
                }
                break;
            case ShapeType::Compound:
            {
                auto compound = physics.CreateCompoundShape();
                for (const auto& nestedData : childData.children)
                    if (auto nested = createChildShape(nestedData))
                        physics.AddChildShape(compound, nested, nestedData.position, nestedData.rotation);
                child = compound;
                break;
            }
        }
        return child;
    };

    std::shared_ptr<ICollisionShape> finalShape;
    switch (shape.type)
    {
        case ShapeType::Box: finalShape = physics.CreateBoxShape(shape.size); break;
        case ShapeType::Sphere: finalShape = physics.CreateSphereShape(shape.radius); break;
        case ShapeType::Capsule: finalShape = physics.CreateCapsuleShape(shape.radius, shape.height); break;
        case ShapeType::Cylinder: finalShape = physics.CreateCylinderShape(shape.radius, shape.height); break;
        case ShapeType::Mesh: finalShape = createMeshShape(entityScale); break;
        case ShapeType::Heightfield:
            if (IsValidHeightfieldSize(shape.heightfieldWidth, shape.heightfieldLength, shape.heightSamples.size()))
                finalShape = physics.CreateHeightfieldShape(shape.heightSamples, shape.heightfieldWidth,
                                                            shape.heightfieldLength, shape.minHeight, shape.maxHeight);
            else
                LOGGER_ERROR("PhysicsSystem") << "Invalid heightfield dimensions or sample count for entity "
                                               << static_cast<uint32_t>(entity) << ".";
            break;
        case ShapeType::Compound:
        {
            auto compound = physics.CreateCompoundShape();
            for (const auto& childData : shape.children)
            {
                if (auto child = createChildShape(childData))
                    physics.AddChildShape(compound, child, childData.position, childData.rotation);
                else
                    LOGGER_ERROR("PhysicsSystem") << "Unable to create compound child shape for entity "
                                                   << static_cast<uint32_t>(entity) << ".";
            }
            finalShape = compound;
            break;
        }
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
        glm::vec3 totalScale(1.0f);
        if (shape.type != ShapeType::Mesh)
        {
            totalScale = entityScale;
            if (shape.type == ShapeType::Heightfield)
                totalScale *= shape.heightfieldScale;
        }

        if (shape.type == ShapeType::Mesh)
        {
            if (auto* meshComp = scene.GetRegistry().try_get<MeshRendererComponent>(entity))
            {
                if (meshComp->model)
                {
                    glm::mat4 rootMtx = meshComp->model->GetRootTransform();
                    glm::vec3 Rs, Rt, Rskew;
                    glm::quat Rr;
                    glm::vec4 Rperspective;
                    if (glm::decompose(rootMtx, Rs, Rr, Rt, Rskew, Rperspective))
                    {
                        totalScale *= Rs;
                    }
                }
            }
        }
        finalShape->SetLocalScaling(totalScale);

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
            rb.body->SetLinearVelocity(rb.initialLinearVelocity);
            rb.body->SetAngularVelocity(rb.initialAngularVelocity);
            physics.AddRigidBody(rb.body.get());
            if (glm::length(rb.initialLinearVelocity) > 0.0001f || glm::length(rb.initialAngularVelocity) > 0.0001f)
            {
                rb.body->Activate(true);
            }
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

    auto& registry = scene.GetRegistry();

    const entt::entity activeCamera = scene.GetActiveCamera();
    if (registry.valid(activeCamera) && registry.all_of<CameraComponent>(activeCamera))
    {
        auto& camera = registry.get<CameraComponent>(activeCamera);
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
