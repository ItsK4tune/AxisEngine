#include <core/logic/service_locator.h>
#include <ecs/logic/entity_manager.h>
#include <ecs/unit/script_component.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <scene/logic/scene_validator.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <engine/platform/logic/io_handler.h>
#include <physics/interface/i_physics_world.h>
#include <physics/interface/i_rigid_body.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>
#include <script/logic/script_registry.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

namespace SceneHandlers
{
void SceneValidator::ValidateParentChildRelationships(
    Scene& scene, const std::map<entt::entity, std::vector<std::string>>& deferredChildren)
{
    if (deferredChildren.empty())
        return;

    auto view = scene.registry.view<InfoComponent>();
    for (const auto& [childEntity, parentNames] : deferredChildren)
    {
        for (const auto& parentName : parentNames)
        {
            entt::entity parentEntity = entt::null;
            for (auto entity : view)
            {
                if (view.get<InfoComponent>(entity).name == parentName)
                {
                    parentEntity = entity;
                    break;
                }
            }

            if (parentEntity != entt::null)
            {
                if (scene.registry.all_of<HierarchyComponent>(childEntity) &&
                    scene.registry.all_of<HierarchyComponent>(parentEntity))
                {
                    EntityManager::AddChild(scene, parentEntity, childEntity, true);
                }
            }
            else
            {
                LOGGER_ERROR("SceneValidator")
                    << "Parent not found: " << parentName << " for Child Entity ID: " << (uint32_t)childEntity;
            }
        }
    }
}

void SceneValidator::ValidateLights(Scene& scene)
{
    auto dirLightView = scene.registry.view<DirectionalLightComponent>();
    bool hasShadowCaster = false;
    entt::entity lastDirLight = entt::null;

    for (auto entity : dirLightView)
    {
        auto& light = dirLightView.get<DirectionalLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            hasShadowCaster = true;
            break;
        }
        if (light.active)
            lastDirLight = entity;
    }
}

void SceneValidator::ValidateCamera(Scene& scene)
{
    if (EntityManager::GetActiveCamera(scene) != entt::null)
        return;

    auto renderableView = scene.registry.view<MeshRendererComponent>();
    bool hasRenderableEntities = false;
    for (auto entity : renderableView)
    {
        hasRenderableEntities = true;
        break;
    }

    if (!hasRenderableEntities)
        return;

    LOGGER_WARN("SceneValidator") << "No Active Camera found in scene! Creating Default Spectator Camera.";

    std::string scriptName = "DefaultCameraController";
    entt::entity camEntity = EntityManager::CreateEntity(scene);

    auto& info = scene.registry.get<InfoComponent>(camEntity);
    info.name = "Default Spectator Camera";
    info.tag = "Default";

    auto& pos = scene.registry.get<PositionComponent>(camEntity);
    pos.value = glm::vec3(0.0f, 2.0f, 10.0f);
    pos.prev = pos.value;

    auto& cam = scene.registry.emplace<CameraComponent>(camEntity);
    cam.isPrimary = true;
    cam.fov = 45.0f;
    cam.nearPlane = 0.1f;
    cam.farPlane = 1000.0f;

    auto& rot = scene.registry.get_or_emplace<RotationComponent>(camEntity);
    rot.value = glm::quatLookAt(glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    rot.prev = rot.value;

    auto io = ServiceLocator::Instance().Resolve<IOHandler>();
    if (io)
    {
        cam.aspectRatio = (float)io->GetMonitorManager().GetWidth() / (float)io->GetMonitorManager().GetHeight();
    }
    else
    {
        cam.aspectRatio = 16.0f / 9.0f;
    }

    auto scriptRegistry = ServiceLocator::Instance().Resolve<ScriptRegistry>();
    auto scriptInstance = scriptRegistry ? scriptRegistry->Create(scriptName) : nullptr;

    if (scriptInstance)
    {
        auto& scriptComp = scene.registry.emplace<ScriptComponent>(camEntity);
        scriptComp.className = scriptName;
        scriptComp.instance = std::move(scriptInstance);
        scriptComp.InstantiateScript = [scriptName]() {
            auto registry = ServiceLocator::Instance().Resolve<ScriptRegistry>();
            return registry ? registry->Create(scriptName) : nullptr;
        };
        scriptComp.DestroyScript = [](ScriptComponent* nsc) {
            nsc->instance.reset();
            nsc->scriptableInstance = nullptr;
            nsc->inputScriptableInstance = nullptr;
        };

        try
        {
            scriptComp.instance->Initialize(camEntity, &scene);
            scriptComp.instance->OnCreate();
            LOGGER_INFO("SceneValidator") << "Attached 'DefaultCameraController' (Engine Fallback) to default camera.";
        }
        catch (const std::exception& e)
        {
            LOGGER_ERROR("SceneValidator") << "DefaultCameraController initialization CRASH: " << e.what();
            scriptComp.instance = nullptr;
        }
    }
    else
    {
        LOGGER_WARN("SceneValidator") << "'DefaultCameraController' script not found! Make sure it is compiled.";
    }
}

void SceneValidator::ValidatePhysicsSync(Scene& scene, IPhysicsWorld* phys)
{
    if (!phys)
        return;
    auto rbView = scene.registry.view<RigidBodyComponent, WorldTransformComponent>();
    for (auto entity : rbView)
    {
        auto& rb = rbView.get<RigidBodyComponent>(entity);
        auto& world = rbView.get<WorldTransformComponent>(entity);

        if (rb.body)
        {
            glm::mat4 worldMatrix = world.worldMatrix;
            glm::vec3 position = glm::vec3(worldMatrix[3]);
            glm::quat rotation = glm::quat_cast(worldMatrix);

            phys->SyncRigidBody(rb.body.get(), position, rotation);
            rb.body->SetUserPointer((void*)(uintptr_t)((uint32_t)entity + 1));

            if (rb.isAttachedToParent && scene.registry.all_of<HierarchyComponent>(entity))
            {
                auto& hier = scene.registry.get<HierarchyComponent>(entity);
                if (scene.registry.valid(hier.parent) && scene.registry.all_of<RigidBodyComponent>(hier.parent))
                {
                    auto& parentRb = scene.registry.get<RigidBodyComponent>(hier.parent);
                    if (parentRb.body)
                    {
                        glm::vec3 pivotA = glm::vec3(0.0f);
                        glm::vec3 pivotB = glm::vec3(0.0f);
                        glm::quat rotA = glm::quat(1, 0, 0, 0);
                        glm::quat rotB = glm::quat(1, 0, 0, 0);

                        auto& parentWorld = scene.registry.get<WorldTransformComponent>(hier.parent);
                        glm::mat4 invParent = glm::inverse(parentWorld.worldMatrix);
                        glm::mat4 localChild = invParent * world.worldMatrix;

                        pivotA = glm::vec3(localChild[3]);
                        rotA = glm::quat_cast(localChild);

                        auto fixedConstraint =
                            phys->CreateFixedConstraint(parentRb.body, rb.body, pivotA, pivotB, rotA, rotB);

                        phys->AddConstraint(fixedConstraint);
                    }
                }
            }
        }
    }
}
}  // namespace SceneHandlers
