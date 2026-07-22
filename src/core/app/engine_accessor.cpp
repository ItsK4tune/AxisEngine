#include <core/app/engine_accessor.h>
#include <ecs/logic/entity.h>
#include <physics/logic/collision_matrix.h>
#include <ecs/unit/physics_components.h>
#include <ecs/logic/physics_system.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_render_state_manager.h>
#include <ecs/interface/i_render_service.h>
#include <resource/unit/shader.h>
#include <resource/unit/model.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/core_components.h>
#include <navigation/unit/navmesh_component.h>
#include <audio/logic/audio_service.h>
#include <core/app/runtime_core.h>
#include <core/logic/config_loader.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/logic/data_manager.h>
#include <core/logic/data_node_serializer.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <ecs/logic/post_process_system.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/input_serializer.h>
#include <platform/logic/input_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <core/logic/config_serializer.h>
#include <scene/logic/scene_serializer.h>
#include <scene/logic/binary_scene_serializer.h>
#include <physics/interface/i_physics_world.h>
#include <physics/logic/constraint_lifecycle.h>

namespace
{
bool CanCreateConstraint(const Scene& scene, const Entity& first, const Entity& second)
{
    return first.GetScene() == &scene && second.GetScene() == &scene && first != second && first.IsValid() &&
           second.IsValid();
}
}  // namespace

void* EngineAccessor::ResolveService(std::type_index type) const
{
    return ServiceLocator::Instance().ResolveByType(type);
}

IBaseSystem* EngineAccessor::ResolveSystem(std::type_index type) const
{
    if (auto* registry = Resolve<ISystemRegistry>())
        return registry->GetSystem(type);
    return nullptr;
}

Scene& EngineAccessor::GetScene() const
{
    return m_ActiveScene ? *m_ActiveScene : ServiceLocator::Instance().Require<Scene>();
}

void EngineAccessor::LoadScene(const std::string& path, bool persistent)
{
    ServiceLocator::Instance().Require<SceneManager>().LoadScene(path, persistent);
}
bool EngineAccessor::LoadInputBindings(const std::string& path)
{
    auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
    return resources && resources->LoadUnified("INPUT", path);
}
bool EngineAccessor::SaveInputBindings(const std::string& path)
{
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        InputSerializer serializer;
        return serializer.Serialize(path, io->GetInputManager());
    }
    return false;
}
bool EngineAccessor::LoadDataNodes(const std::string& path)
{
    if (auto* dm = ServiceLocator::Instance().Resolve<DataManager>())
    {
        DataNodeSerializer serializer;
        std::unordered_map<std::string, DataNode> data;
        if (!serializer.Deserialize(path, data))
            return false;
        dm->ReplaceDataNodes(std::move(data));
        return true;
    }
    return false;
}
bool EngineAccessor::SaveDataNodes(const std::string& path)
{
    if (auto* dm = ServiceLocator::Instance().Resolve<DataManager>())
    {
        DataNodeSerializer serializer;
        const auto data = dm->GetDataNodes();
        return serializer.Serialize(path, data);
    }
    return false;
}
void EngineAccessor::SetDataNode(const std::string& key, const DataNode& data)
{
    if (auto* manager = Resolve<DataManager>())
        manager->SetDataNode(key, data);
}
DataNode EngineAccessor::GetDataNode(const std::string& key) const
{
    if (auto* manager = Resolve<DataManager>())
        return manager->GetDataNode(key);
    return {};
}
bool EngineAccessor::HasDataNode(const std::string& key) const
{
    if (auto* manager = Resolve<DataManager>())
        return manager->HasDataNode(key);
    return false;
}
void EngineAccessor::RemoveDataNode(const std::string& key)
{
    if (auto* manager = Resolve<DataManager>())
        manager->RemoveDataNode(key);
}
bool EngineAccessor::LoadConfig(const std::string& path)
{
    auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
    return resources && resources->LoadUnified("CONFIG", path);
}
bool EngineAccessor::SaveConfig(const std::string& path)
{
    if (auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>())
    {
        ConfigSerializer serializer;
        return serializer.Serialize(path, cm->GetConfig());
    }
    return false;
}
bool EngineAccessor::SaveScene(const std::string& path, const std::string& sceneName)
{
    if (path.ends_with(".axsb"))
    {
        BinarySceneSerializer serializer;
        return serializer.Serialize(path, GetScene());
    }
    else
    {
        auto* rm = Resolve<ResourceManager>();
        auto* phys = Resolve<IPhysicsWorld>();
        auto* audio = Resolve<AudioService>();
        if (rm)
        {
            SceneSerializer serializer(*rm, phys, audio);
            return serializer.Serialize(path, GetScene(), sceneName);
        }
    }
    return false;
}

void EngineAccessor::SetCursorMode(CursorMode mode)
{
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>())
        io->GetMouse().SetCursorMode(mode);
}

void EngineAccessor::LoadLanguage(const std::string& path, const std::string& name)
{
    if (auto* loc = Resolve<ILocalizationService>())
        loc->LoadLanguage(path, name);
}

void EngineAccessor::SetLanguage(const std::string& name)
{
    if (auto* loc = Resolve<ILocalizationService>())
        loc->SetLanguage(name);
}

std::string EngineAccessor::GetLanguage() const
{
    if (auto* loc = Resolve<ILocalizationService>())
        return loc->GetLanguage();
    return "";
}

std::string EngineAccessor::GetTranslation(const std::string& key) const
{
    if (auto* loc = Resolve<ILocalizationService>())
        return loc->Get(key);
    return "[MISSING: " + key + "]";
}

void EngineAccessor::QueueLoadScene(const std::string& path, bool persistent)
{
    ServiceLocator::Instance().Require<SceneManager>().QueueLoadScene(path, persistent);
}
void EngineAccessor::QueueUnloadScene(const std::string& path)
{
    ServiceLocator::Instance().Require<SceneManager>().QueueUnloadScene(path);
}
void EngineAccessor::UnloadScene(const std::string& path)
{
    ServiceLocator::Instance().Require<SceneManager>().UnloadScene(path);
}
void EngineAccessor::UnloadScene(const SceneRecord* rec)
{
    ServiceLocator::Instance().Require<SceneManager>().UnloadScene(rec);
}
void EngineAccessor::ChangeScene(const std::string& path)
{
    ServiceLocator::Instance().Require<SceneManager>().ChangeScene(path);
}
void EngineAccessor::QueueChangeScene(const std::string& path)
{
    ServiceLocator::Instance().Require<SceneManager>().QueueChangeScene(path);
}
void EngineAccessor::PopScene()
{
    ServiceLocator::Instance().Require<SceneManager>().PopScene();
}
void EngineAccessor::QueuePopScene()
{
    ServiceLocator::Instance().Require<SceneManager>().QueuePopScene();
}
std::vector<const SceneRecord*> EngineAccessor::GetScenes()
{
    return ServiceLocator::Instance().Require<SceneManager>().GetScenes();
}
bool EngineAccessor::IsSceneLoaded(const std::string& path)
{
    return ServiceLocator::Instance().Require<SceneManager>().IsLoaded(path);
}

void EngineAccessor::LogAllScenes()
{
    ServiceLocator::Instance().Require<SceneManager>().LogAllScenes();
}

void EngineAccessor::EnableSystem(const std::string& systemName, bool enable)
{
    if (auto* systemManager = Resolve<ISystemRegistry>())
    {
        if (auto* system = systemManager->GetSystem(systemName))
        {
            system->SetEnabled(enable);
        }
    }

    EventManager::Instance().Publish(SystemEnabledEvent{systemName, enable});
}

void EngineAccessor::EnableSystem(SystemId systemId, bool enable)
{
    if (auto* systemManager = Resolve<ISystemRegistry>())
    {
        if (auto* system = systemManager->GetSystem(systemId))
        {
            system->SetEnabled(enable);
            EventManager::Instance().Publish(SystemEnabledEvent{system->GetName(), enable});
        }
    }
}

void EngineAccessor::EnableLogic(bool enable)
{
    EnableScript(enable);
    EnableAnimation(enable);
    EnableVideo(enable);
    EnableParticle(enable);
    EnableNavigation(enable);
}

bool EngineAccessor::GetAction(const std::string& name) const
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    return io ? io->GetInputManager().GetAction(name) : false;
}
bool EngineAccessor::GetActionDown(const std::string& name) const
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    return io ? io->GetInputManager().GetActionDown(name) : false;
}
bool EngineAccessor::GetActionUp(const std::string& name) const
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    return io ? io->GetInputManager().GetActionUp(name) : false;
}
float EngineAccessor::GetAxis(const std::string& name) const
{
    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    return io ? io->GetInputManager().GetAxis(name) : 0.0f;
}

void EngineAccessor::SetTimeScale(float scale)
{
    ServiceLocator::Instance().Require<RuntimeCore>().GetEngineLoop().SetTimeScale(scale);
}
float EngineAccessor::GetTimeScale() const
{
    return ServiceLocator::Instance().Require<RuntimeCore>().GetEngineLoop().GetTimeScale();
}
float EngineAccessor::GetRealDeltaTime() const
{
    return ServiceLocator::Instance().Require<RuntimeCore>().GetEngineLoop().GetRealDeltaTime();
}

AppConfig EngineAccessor::GetConfig() const
{
    return ServiceLocator::Instance().Require<ConfigManager>().GetConfig();
}
void EngineAccessor::ApplyConfig(const AppConfig& config)
{
    ServiceLocator::Instance().Require<ConfigManager>().UpdateConfig(config);
}

void EngineAccessor::SetPhysicsGravity(const glm::vec3& gravity)
{
    if (auto* physics = Resolve<IPhysicsWorld>())
    {
        physics->SetGravity(gravity);
    }
}

void EngineAccessor::SetPhysicsSolverIterations(int iterations)
{
    if (auto* physics = Resolve<IPhysicsWorld>())
    {
        physics->SetSolverIterations(iterations);
    }
}

void EngineAccessor::IgnoreTagCollision(const std::string& tag1, const std::string& tag2)
{
    if (auto* matrix = Resolve<CollisionMatrix>())
    {
        matrix->IgnoreTagCollision(tag1, tag2);
    }
}

void EngineAccessor::ForcePhysicsUpdate(float dt)
{
    GetSystem<PhysicsSystem>().FixedUpdate(GetScene(), dt);
}

void EngineAccessor::CreateHingeConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA,
                                           const glm::vec3& pivotB, const glm::vec3& axisA, const glm::vec3& axisB)
{
    auto* physics = Resolve<IPhysicsWorld>();
    if (!physics)
        return;

    auto& scene = GetScene();
    if (!CanCreateConstraint(scene, entityA, entityB))
        return;
    auto& reg = scene.GetRegistry();
    if (reg.all_of<RigidBodyComponent>(entityA) && reg.all_of<RigidBodyComponent>(entityB))
    {
        auto& rbA = reg.get<RigidBodyComponent>(entityA);
        auto& rbB = reg.get<RigidBodyComponent>(entityB);
        if (rbA.body && rbB.body)
        {
            auto constraint = physics->CreateHingeConstraint(rbA.body, rbB.body, pivotA, pivotB, axisA, axisB);
            if (constraint)
            {
                physics->AddConstraint(constraint);
                PhysicsConstraintLifecycle::Track(constraint, rbA, rbB);
            }
        }
    }
}

void EngineAccessor::CreatePointToPointConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA,
                                                  const glm::vec3& pivotB)
{
    auto* physics = Resolve<IPhysicsWorld>();
    if (!physics)
        return;

    auto& scene = GetScene();
    if (!CanCreateConstraint(scene, entityA, entityB))
        return;
    auto& reg = scene.GetRegistry();
    if (reg.all_of<RigidBodyComponent>(entityA) && reg.all_of<RigidBodyComponent>(entityB))
    {
        auto& rbA = reg.get<RigidBodyComponent>(entityA);
        auto& rbB = reg.get<RigidBodyComponent>(entityB);
        if (rbA.body && rbB.body)
        {
            auto constraint = physics->CreatePoint2PointConstraint(rbA.body, rbB.body, pivotA, pivotB);
            if (constraint)
            {
                physics->AddConstraint(constraint);
                PhysicsConstraintLifecycle::Track(constraint, rbA, rbB);
            }
        }
    }
}

void EngineAccessor::CreateFixedConstraint(Entity entityA, Entity entityB, const glm::vec3& pivotA,
                                           const glm::vec3& pivotB, const glm::quat& rotA, const glm::quat& rotB)
{
    auto* physics = Resolve<IPhysicsWorld>();
    if (!physics)
        return;

    auto& scene = GetScene();
    if (!CanCreateConstraint(scene, entityA, entityB))
        return;
    auto& reg = scene.GetRegistry();
    if (reg.all_of<RigidBodyComponent>(entityA) && reg.all_of<RigidBodyComponent>(entityB))
    {
        auto& rbA = reg.get<RigidBodyComponent>(entityA);
        auto& rbB = reg.get<RigidBodyComponent>(entityB);
        if (rbA.body && rbB.body)
        {
            auto constraint = physics->CreateFixedConstraint(rbA.body, rbB.body, pivotA, pivotB, rotA, rotB);
            if (constraint)
            {
                physics->AddConstraint(constraint);
                PhysicsConstraintLifecycle::Track(constraint, rbA, rbB);
            }
        }
    }
}

void EngineAccessor::ClearStencilBuffer()
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->Clear(BufferBit::Stencil);
    }
}

void EngineAccessor::ClearDepthBuffer()
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->Clear(BufferBit::Depth);
    }
}

void EngineAccessor::SetRenderStateEnabled(ServerCapability capability, bool enable)
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        if (enable)
            context->GetRenderStateManager().Enable(capability);
        else
            context->GetRenderStateManager().Disable(capability);
    }
}

void EngineAccessor::SetStencilMask(uint32_t mask)
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->GetRenderStateManager().SetStencilMask(mask);
    }
}

void EngineAccessor::SetStencilFunc(CompareFunc func, int ref, uint32_t mask)
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->GetRenderStateManager().SetStencilFunc(func, ref, mask);
    }
}

void EngineAccessor::SetStencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass)
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->GetRenderStateManager().SetStencilOp(sfail, dpfail, dppass);
    }
}

void EngineAccessor::SetDepthFunc(CompareFunc func)
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->GetRenderStateManager().SetDepthFunc(func);
    }
}

void EngineAccessor::SetColorWriteMask(bool r, bool g, bool b, bool a)
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->GetRenderStateManager().SetColorMask(r, g, b, a);
    }
}

void EngineAccessor::SetDepthWriteMask(bool enable)
{
    if (auto* context = Resolve<IGraphicsContext>())
    {
        context->GetRenderStateManager().SetDepthMask(enable);
    }
}

void EngineAccessor::GetCameraRenderState(glm::vec3& outPos, glm::mat4& outView, glm::mat4& outProj, float& outNear,
                                          float& outFar)
{
    if (auto* render = Resolve<IRenderService>())
    {
        outPos = render->GetCameraPosition();
        outView = render->GetViewMatrix();
        outProj = render->GetProjectionMatrix();
        outNear = render->GetNearPlane();
        outFar = render->GetFarPlane();
    }
}

void EngineAccessor::SetCameraRenderState(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos,
                                          float nearPlane, float farPlane)
{
    if (auto* render = Resolve<IRenderService>())
    {
        GPUCameraData camData;
        std::memcpy(camData.projection, &proj[0][0], 16 * sizeof(float));
        std::memcpy(camData.view, &view[0][0], 16 * sizeof(float));
        std::memcpy(camData.viewPos, &pos[0], 3 * sizeof(float));
        camData.viewPos[3] = 1.0f;
        glm::mat4 invProj = glm::inverse(proj);
        glm::mat4 invView = glm::inverse(view);
        std::memcpy(camData.invProjection, &invProj[0][0], 16 * sizeof(float));
        std::memcpy(camData.invView, &invView[0][0], 16 * sizeof(float));
        std::memcpy(camData.stableProjection, &proj[0][0], 16 * sizeof(float));
        std::memcpy(camData.invStableProjection, &invProj[0][0], 16 * sizeof(float));

        render->UploadCameraUBO(camData);
        render->RestoreCameraState(view, proj, pos, nearPlane, farPlane);
    }
}

void EngineAccessor::DrawEntityMesh(Entity entity, const std::string& shaderName, const glm::mat4& customWorldTransform,
                                    const glm::vec4& color, float metallic, float roughness, float ao)
{
    auto& reg = GetScene().GetRegistry();
    if (reg.all_of<MeshRendererComponent>(entity))
    {
        auto& mesh = reg.get<MeshRendererComponent>(entity);
        if (mesh.model)
        {
            auto* resMgr = Resolve<ResourceManager>();
            if (resMgr)
            {
                auto shader = resMgr->GetShader(shaderName);
                if (shader)
                {
                    shader->use();
                    glm::mat4 modelMtx = customWorldTransform * mesh.model->GetRootTransform();
                    shader->setMat4("u_Model", modelMtx);
                    shader->setVec4("u_BaseColor", color);
                    shader->setFloat("u_Metallic", metallic);
                    shader->setFloat("u_Roughness", roughness);
                    shader->setFloat("u_AO", ao);

                    mesh.model->Draw(*shader, true);
                }
            }
        }
    }
}

void EngineAccessor::ConfigurePostProcessing(bool hdr, bool bloom, float threshold, float intensity, float radius,
                                             float exposure, float gamma, int tonemappingMode)
{
    if (auto* sysMgr = Resolve<ISystemRegistry>())
    {
        if (auto* ppSys = dynamic_cast<PostProcessSystem*>(sysMgr->GetSystem("PostProcessSystem")))
        {
            auto& pipeline = ppSys->GetPipeline();
            pipeline.SetHDREnabled(hdr);
            pipeline.SetBloomEnabled(bloom);
            pipeline.SetBloomThreshold(threshold);
            pipeline.SetBloomIntensity(intensity);
            pipeline.SetBloomRadius(radius);
            pipeline.SetExposure(exposure);
            pipeline.SetGamma(gamma);
            pipeline.SetTonemappingMode(tonemappingMode);
        }
    }
}

std::vector<Entity> EngineAccessor::GetEntitiesWithName(const std::string& name) const
{
    std::vector<Entity> result;
    if (m_ActiveScene)
    {
        auto view = m_ActiveScene->GetRegistry().view<InfoComponent>();
        for (auto entity : view)
        {
            if (view.get<InfoComponent>(entity).name == name)
            {
                result.push_back(Entity(entity, m_ActiveScene));
            }
        }
    }
    return result;
}

std::vector<Entity> EngineAccessor::GetEntitiesWithNamePrefix(const std::string& prefix) const
{
    std::vector<Entity> result;
    if (m_ActiveScene)
    {
        auto view = m_ActiveScene->GetRegistry().view<InfoComponent>();
        for (auto entity : view)
        {
            if (view.get<InfoComponent>(entity).name.rfind(prefix, 0) == 0)
            {
                result.push_back(Entity(entity, m_ActiveScene));
            }
        }
    }
    return result;
}

std::vector<Entity> EngineAccessor::GetCameraEntities() const
{
    std::vector<Entity> result;
    if (m_ActiveScene)
    {
        auto view = m_ActiveScene->GetRegistry().view<CameraComponent>();
        for (auto entity : view)
        {
            result.push_back(Entity(entity, m_ActiveScene));
        }
    }
    return result;
}

size_t EngineAccessor::GetEntityCount() const
{
    if (m_ActiveScene)
    {
        return m_ActiveScene->GetRegistry().view<InfoComponent>().size();
    }
    return 0;
}

void EngineAccessor::UpdateNavMeshHeightsAndTags(
    std::function<void(const glm::vec3& pos, glm::vec3& outPos, std::string& outTag)> modifier)
{
    if (m_ActiveScene && modifier)
    {
        auto view = m_ActiveScene->GetRegistry().view<NavMeshComponent>();
        for (auto entity : view)
        {
            auto& navMesh = view.get<NavMeshComponent>(entity);
            for (auto& tri : navMesh.triangles)
            {
                modifier(tri.center, tri.center, tri.tag);
            }
            for (auto& node : navMesh.nodes)
            {
                modifier(node.position, node.position, node.tag);
            }
        }
    }
}
