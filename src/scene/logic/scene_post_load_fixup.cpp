#include <scene/logic/scene_post_load_fixup.h>

#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/interface/i_script_registry.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/script_component.h>
#include <engine/platform/logic/io_handler.h>
#include <physics/interface/i_physics_world.h>
#include <physics/interface/i_rigid_body.h>
#include <platform/logic/monitor_manager.h>
#include <scene/logic/scene.h>
#include <algorithm>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace
{
bool IsInScope(entt::entity entity, const std::vector<entt::entity>& scope)
{
    return scope.empty() || std::find(scope.begin(), scope.end(), entity) != scope.end();
}
}  // namespace

namespace SceneHandlers
{
void ScenePostLoadFixup::ResolveParentChildRelationships(
    Scene& scene, const std::map<entt::entity, std::vector<std::string>>& deferredChildren)
{
    if (deferredChildren.empty())
        return;

    auto view = scene.View<InfoComponent>();
    for (const auto& [childEntity, parentNames] : deferredChildren)
    {
        if (!scene.IsValid(childEntity))
            continue;
        for (const auto& parentName : parentNames)
        {
            entt::entity parentEntity = entt::null;
            size_t matches = 0;
            for (auto entity : view)
            {
                if (view.get<InfoComponent>(entity).name == parentName)
                {
                    if (parentEntity == entt::null)
                        parentEntity = entity;
                    ++matches;
                }
            }

            if (matches > 1)
                LOGGER_WARN("ScenePostLoadFixup")
                    << "Ambiguous parent name '" << parentName << "' matched " << matches
                    << " entities; using the first registry match for child " << static_cast<uint32_t>(childEntity);

            if (parentEntity != entt::null)
            {
                if (scene.HasAllComponents<HierarchyComponent>(childEntity) &&
                    scene.HasAllComponents<HierarchyComponent>(parentEntity))
                    scene.AddChild(parentEntity, childEntity, true);
            }
            else
            {
                LOGGER_ERROR("ScenePostLoadFixup") << "Parent not found: " << parentName
                                                   << " for Child Entity ID: " << static_cast<uint32_t>(childEntity);
            }
        }
    }
}

bool ScenePostLoadFixup::EnsureFallbackCamera(Scene& scene)
{
    if (scene.GetActiveCamera() != entt::null || scene.View<MeshRendererComponent>().empty())
        return false;

    LOGGER_WARN("ScenePostLoadFixup") << "No camera found in renderable scene; creating default spectator camera.";

    constexpr const char* scriptName = "DefaultCameraController";
    entt::entity cameraEntity = scene.CreateEntity();
    auto& info = scene.GetComponent<InfoComponent>(cameraEntity);
    info.name = "Default Spectator Camera";
    info.tag = "Default";

    auto& position = scene.GetComponent<PositionComponent>(cameraEntity);
    position.value = glm::vec3(0.0f, 2.0f, 10.0f);
    position.prev = position.value;

    auto& camera = scene.AddComponent<CameraComponent>(cameraEntity);
    camera.isPrimary = true;
    camera.fov = 45.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 1000.0f;

    auto& rotation = scene.GetOrAddComponent<RotationComponent>(cameraEntity);
    rotation.value = glm::quatLookAt(glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    rotation.prev = rotation.value;

    if (auto io = ServiceLocator::Instance().Resolve<IOHandler>())
    {
        const int height = io->GetMonitorManager().GetHeight();
        camera.aspectRatio = height > 0
                                 ? static_cast<float>(io->GetMonitorManager().GetWidth()) / static_cast<float>(height)
                                 : 16.0f / 9.0f;
    }
    else
    {
        camera.aspectRatio = 16.0f / 9.0f;
    }

    auto scriptRegistry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
    auto scriptInstance = scriptRegistry ? scriptRegistry->Create(scriptName) : nullptr;
    if (!scriptInstance)
    {
        LOGGER_WARN("ScenePostLoadFixup") << "'DefaultCameraController' script is unavailable.";
        return true;
    }

    auto& script = scene.AddComponent<ScriptComponent>(cameraEntity);
    script.className = scriptName;
    script.instance = std::move(scriptInstance);
    script.InstantiateScript = []() {
        auto registry = ServiceLocator::Instance().Resolve<IScriptRegistry>();
        return registry ? registry->Create("DefaultCameraController") : nullptr;
    };
    script.DestroyScript = [](ScriptComponent* component) {
        component->instance.reset();
        component->scriptableInstance = nullptr;
        component->inputScriptableInstance = nullptr;
    };

    try
    {
        script.instance->Initialize(cameraEntity, &scene);
        script.instance->OnCreate();
    }
    catch (const std::exception& error)
    {
        LOGGER_ERROR("ScenePostLoadFixup") << "DefaultCameraController initialization failed: " << error.what();
        script.instance.reset();
    }
    return true;
}

void ScenePostLoadFixup::InitializePhysicsBindings(Scene& scene, IPhysicsWorld* physics,
                                                   const std::vector<entt::entity>& entityScope)
{
    if (!physics)
        return;

    auto view = scene.View<RigidBodyComponent, WorldTransformComponent>();
    for (auto entity : view)
    {
        if (!IsInScope(entity, entityScope))
            continue;
        auto& rigidBody = view.get<RigidBodyComponent>(entity);
        const auto& world = view.get<WorldTransformComponent>(entity);
        if (!rigidBody.body)
            continue;

        const glm::vec3 position = glm::vec3(world.worldMatrix[3]);
        const glm::quat rotation = glm::quat_cast(world.worldMatrix);
        physics->SyncRigidBody(rigidBody.body.get(), position, rotation);
        rigidBody.body->SetUserPointer(
            reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32_t>(entity) + 1)));

        if (!rigidBody.isAttachedToParent)
            continue;
        const auto* hierarchy = scene.TryGetComponent<HierarchyComponent>(entity);
        if (!hierarchy || !scene.IsValid(hierarchy->parent))
            continue;
        auto* parentRigidBody = scene.TryGetComponent<RigidBodyComponent>(hierarchy->parent);
        auto* parentWorld = scene.TryGetComponent<WorldTransformComponent>(hierarchy->parent);
        if (!parentRigidBody || !parentRigidBody->body || !parentWorld)
            continue;

        const glm::mat4 localChild = glm::inverse(parentWorld->worldMatrix) * world.worldMatrix;
        auto constraint =
            physics->CreateFixedConstraint(parentRigidBody->body, rigidBody.body, glm::vec3(localChild[3]),
                                           glm::vec3(0.0f), glm::quat_cast(localChild), glm::quat(1, 0, 0, 0));
        physics->AddConstraint(constraint);
    }
}
}  // namespace SceneHandlers
