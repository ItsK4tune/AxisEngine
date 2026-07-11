#include <scene/logic/scene_validator.h>

#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/render_components.h>
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>
#include <scene/logic/scene.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>

namespace
{
bool IsInScope(entt::entity entity, const SceneValidationOptions& options)
{
    return options.entityScope.empty() ||
           std::find(options.entityScope.begin(), options.entityScope.end(), entity) != options.entityScope.end();
}

bool IsFinite(float value)
{
    return std::isfinite(value);
}

bool IsFinite(const glm::vec3& value)
{
    return IsFinite(value.x) && IsFinite(value.y) && IsFinite(value.z);
}

bool IsFinite(const glm::quat& value)
{
    return IsFinite(value.x) && IsFinite(value.y) && IsFinite(value.z) && IsFinite(value.w);
}

void Add(SceneValidationResult& result, SceneValidationSeverity severity, const char* code, entt::entity entity,
         const char* component, const std::string& message)
{
    result.Add(severity, code, entity, component, message);
}

void ValidateHierarchy(const Scene& scene, const SceneValidationOptions& options, SceneValidationResult& result)
{
    const auto& registry = scene.GetRegistry();
    auto view = scene.View<HierarchyComponent>();
    for (auto entity : view)
    {
        if (!IsInScope(entity, options))
            continue;

        const auto& hierarchy = view.get<HierarchyComponent>(entity);
        if (hierarchy.parent == entity)
        {
            Add(result, SceneValidationSeverity::Fatal, "HIERARCHY_SELF_PARENT", entity, "HierarchyComponent",
                "Entity cannot be its own parent.");
        }
        else if (hierarchy.parent != entt::null)
        {
            if (!registry.valid(hierarchy.parent))
            {
                Add(result, SceneValidationSeverity::Fatal, "HIERARCHY_INVALID_PARENT", entity, "HierarchyComponent",
                    "Parent references an invalid entity.");
            }
            else if (const auto* parentHierarchy = registry.try_get<HierarchyComponent>(hierarchy.parent))
            {
                if (std::find(parentHierarchy->children.begin(), parentHierarchy->children.end(), entity) ==
                    parentHierarchy->children.end())
                {
                    Add(result, SceneValidationSeverity::Error, "HIERARCHY_PARENT_CHILD_MISMATCH", entity,
                        "HierarchyComponent", "Parent does not contain this entity in its child list.");
                }
            }
            else
            {
                Add(result, SceneValidationSeverity::Error, "HIERARCHY_PARENT_MISSING_COMPONENT", entity,
                    "HierarchyComponent", "Parent entity has no HierarchyComponent.");
            }
        }

        std::unordered_set<uint32_t> uniqueChildren;
        for (entt::entity child : hierarchy.children)
        {
            if (!uniqueChildren.insert(static_cast<uint32_t>(child)).second)
            {
                Add(result, SceneValidationSeverity::Error, "HIERARCHY_DUPLICATE_CHILD", entity, "HierarchyComponent",
                    "Child list contains a duplicate entity.");
                continue;
            }
            if (!registry.valid(child))
            {
                Add(result, SceneValidationSeverity::Error, "HIERARCHY_INVALID_CHILD", entity, "HierarchyComponent",
                    "Child list references an invalid entity.");
                continue;
            }
            const auto* childHierarchy = registry.try_get<HierarchyComponent>(child);
            if (!childHierarchy || childHierarchy->parent != entity)
            {
                Add(result, SceneValidationSeverity::Error, "HIERARCHY_CHILD_PARENT_MISMATCH", entity,
                    "HierarchyComponent", "Child does not reference this entity as its parent.");
            }
        }

        std::unordered_set<uint32_t> ancestors;
        entt::entity cursor = entity;
        while (cursor != entt::null && registry.valid(cursor))
        {
            if (!ancestors.insert(static_cast<uint32_t>(cursor)).second)
            {
                Add(result, SceneValidationSeverity::Fatal, "HIERARCHY_CYCLE", entity, "HierarchyComponent",
                    "Hierarchy contains a parent cycle.");
                break;
            }
            const auto* cursorHierarchy = registry.try_get<HierarchyComponent>(cursor);
            if (!cursorHierarchy)
                break;
            cursor = cursorHierarchy->parent;
        }
    }
}

void ValidateCameras(const Scene& scene, const SceneValidationOptions& options, SceneValidationResult& result)
{
    size_t cameraCount = 0;
    size_t primaryCount = 0;
    auto view = scene.View<CameraComponent>();
    for (auto entity : view)
    {
        ++cameraCount;
        const auto& camera = view.get<CameraComponent>(entity);
        if (camera.isPrimary)
            ++primaryCount;
        if (!IsInScope(entity, options))
            continue;

        if (!scene.HasAllComponents<PositionComponent, RotationComponent, WorldTransformComponent>(entity))
        {
            Add(result, SceneValidationSeverity::Error, "CAMERA_MISSING_TRANSFORM", entity, "CameraComponent",
                "Camera requires Position, Rotation and WorldTransform components.");
        }
        if (!IsFinite(camera.fov) || camera.fov <= 0.0f || camera.fov >= 180.0f)
            Add(result, SceneValidationSeverity::Error, "CAMERA_INVALID_FOV", entity, "CameraComponent",
                "Camera FOV must be finite and between 0 and 180 degrees.");
        if (!IsFinite(camera.nearPlane) || !IsFinite(camera.farPlane) || camera.nearPlane <= 0.0f ||
            camera.farPlane <= camera.nearPlane)
            Add(result, SceneValidationSeverity::Error, "CAMERA_INVALID_CLIP_PLANES", entity, "CameraComponent",
                "Camera clip planes must satisfy 0 < near < far.");
        if (!IsFinite(camera.aspectRatio) || camera.aspectRatio <= 0.0f)
            Add(result, SceneValidationSeverity::Error, "CAMERA_INVALID_ASPECT", entity, "CameraComponent",
                "Camera aspect ratio must be finite and positive.");
    }

    if (primaryCount > 1)
        Add(result, SceneValidationSeverity::Warning, "CAMERA_MULTIPLE_PRIMARY", entt::null, "CameraComponent",
            "Scene contains multiple primary cameras; selection depends on registry iteration order.");

    if (cameraCount == 0 && !scene.View<MeshRendererComponent>().empty())
        Add(result, SceneValidationSeverity::Warning, "CAMERA_MISSING_RENDERABLE_SCENE", entt::null, "CameraComponent",
            "Renderable scene has no camera; runtime fallback may be created.");
}

void ValidateLightScalars(SceneValidationResult& result, entt::entity entity, const char* component,
                          const glm::vec3& color, float intensity, float ambient, float diffuse, float specular)
{
    if (!IsFinite(color) || !IsFinite(intensity) || !IsFinite(ambient) || !IsFinite(diffuse) || !IsFinite(specular))
        Add(result, SceneValidationSeverity::Error, "LIGHT_NON_FINITE_VALUE", entity, component,
            "Light color and intensity terms must contain only finite values.");
    if (IsFinite(intensity) && intensity < 0.0f)
        Add(result, SceneValidationSeverity::Error, "LIGHT_NEGATIVE_INTENSITY", entity, component,
            "Light intensity cannot be negative.");
}

void ValidateLights(const Scene& scene, const SceneValidationOptions& options, SceneValidationResult& result)
{
    int directionalShadows = 0;
    int pointShadows = 0;
    int spotShadows = 0;

    auto directionalView = scene.View<DirectionalLightComponent>();
    for (auto entity : directionalView)
    {
        const auto& light = directionalView.get<DirectionalLightComponent>(entity);
        const auto* info = scene.TryGetComponent<InfoComponent>(entity);
        if (light.active && light.isCastShadow && (!info || info->isActive))
            ++directionalShadows;
        if (!IsInScope(entity, options))
            continue;
        if (!info)
            Add(result, SceneValidationSeverity::Error, "LIGHT_MISSING_INFO", entity, "DirectionalLightComponent",
                "Directional light requires InfoComponent to enter the render queue.");
        ValidateLightScalars(result, entity, "DirectionalLightComponent", light.color, light.intensity, light.ambient,
                             light.diffuse, light.specular);
        if (!scene.HasAllComponents<RotationComponent>(entity) &&
            (!IsFinite(light.direction) || glm::dot(light.direction, light.direction) <= 0.000001f))
            Add(result, SceneValidationSeverity::Error, "LIGHT_INVALID_DIRECTION", entity, "DirectionalLightComponent",
                "Directional light without RotationComponent needs a finite non-zero direction.");
    }

    auto pointView = scene.View<PointLightComponent>();
    for (auto entity : pointView)
    {
        const auto& light = pointView.get<PointLightComponent>(entity);
        const auto* info = scene.TryGetComponent<InfoComponent>(entity);
        if (light.active && light.isCastShadow && (!info || info->isActive))
            ++pointShadows;
        if (!IsInScope(entity, options))
            continue;
        if (!info)
            Add(result, SceneValidationSeverity::Error, "LIGHT_MISSING_INFO", entity, "PointLightComponent",
                "Point light requires InfoComponent to enter the render queue.");
        if (!scene.HasAllComponents<PositionComponent>(entity))
            Add(result, SceneValidationSeverity::Error, "LIGHT_MISSING_POSITION", entity, "PointLightComponent",
                "Point light requires PositionComponent.");
        ValidateLightScalars(result, entity, "PointLightComponent", light.color, light.intensity, light.ambient,
                             light.diffuse, light.specular);
        if (!IsFinite(light.radius) || light.radius <= 0.0f)
            Add(result, SceneValidationSeverity::Error, "LIGHT_INVALID_RADIUS", entity, "PointLightComponent",
                "Point light radius must be finite and positive.");
        if (!IsFinite(light.constant) || !IsFinite(light.linear) || !IsFinite(light.quadratic) ||
            (light.constant == 0.0f && light.linear == 0.0f && light.quadratic == 0.0f))
            Add(result, SceneValidationSeverity::Error, "LIGHT_INVALID_ATTENUATION", entity, "PointLightComponent",
                "Point light attenuation must be finite and cannot have all terms equal to zero.");
    }

    auto spotView = scene.View<SpotLightComponent>();
    for (auto entity : spotView)
    {
        const auto& light = spotView.get<SpotLightComponent>(entity);
        const auto* info = scene.TryGetComponent<InfoComponent>(entity);
        if (light.active && light.isCastShadow && (!info || info->isActive))
            ++spotShadows;
        if (!IsInScope(entity, options))
            continue;
        if (!info)
            Add(result, SceneValidationSeverity::Error, "LIGHT_MISSING_INFO", entity, "SpotLightComponent",
                "Spot light requires InfoComponent to enter the render queue.");
        if (!scene.HasAllComponents<PositionComponent>(entity))
            Add(result, SceneValidationSeverity::Error, "LIGHT_MISSING_POSITION", entity, "SpotLightComponent",
                "Spot light requires PositionComponent.");
        if (!scene.HasAllComponents<RotationComponent>(entity))
            Add(result, SceneValidationSeverity::Error, "LIGHT_MISSING_ROTATION", entity, "SpotLightComponent",
                "Spot light requires RotationComponent.");
        ValidateLightScalars(result, entity, "SpotLightComponent", light.color, light.intensity, light.ambient,
                             light.diffuse, light.specular);
        if (!IsFinite(light.radius) || light.radius <= 0.0f)
            Add(result, SceneValidationSeverity::Error, "LIGHT_INVALID_RADIUS", entity, "SpotLightComponent",
                "Spot light radius must be finite and positive.");
        if (!IsFinite(light.constant) || !IsFinite(light.linear) || !IsFinite(light.quadratic) ||
            (light.constant == 0.0f && light.linear == 0.0f && light.quadratic == 0.0f))
            Add(result, SceneValidationSeverity::Error, "LIGHT_INVALID_ATTENUATION", entity, "SpotLightComponent",
                "Spot light attenuation must be finite and cannot have all terms equal to zero.");
        if (!IsFinite(light.cutOff) || !IsFinite(light.outerCutOff) || light.cutOff < -1.0f || light.cutOff > 1.0f ||
            light.outerCutOff < -1.0f || light.outerCutOff > 1.0f || light.outerCutOff > light.cutOff)
            Add(result, SceneValidationSeverity::Error, "LIGHT_INVALID_SPOT_CUTOFF", entity, "SpotLightComponent",
                "Spot cutoff cosines must satisfy -1 <= outerCutOff <= cutOff <= 1.");
    }

    if (!options.validateRenderCapabilities)
        return;
    const auto& caps = options.renderCapabilities;
    const int directionalLimit = caps.singleDirectionalShadow ? 1 : caps.directionalShadowLimit;
    if (directionalShadows > directionalLimit)
        Add(result, SceneValidationSeverity::Warning, "LIGHT_DIRECTIONAL_SHADOW_LIMIT", entt::null,
            "DirectionalLightComponent",
            std::to_string(directionalShadows) + " directional lights request shadows, but the renderer supports " +
                std::to_string(directionalLimit) + " in the current mode.");
    if (pointShadows > caps.pointShadowLimit)
        Add(result, SceneValidationSeverity::Warning, "LIGHT_POINT_SHADOW_LIMIT", entt::null, "PointLightComponent",
            std::to_string(pointShadows) + " point lights request shadows, but the renderer supports " +
                std::to_string(caps.pointShadowLimit) + ".");
    if (spotShadows > caps.spotShadowLimit)
        Add(result, SceneValidationSeverity::Warning, "LIGHT_SPOT_SHADOW_LIMIT", entt::null, "SpotLightComponent",
            std::to_string(spotShadows) + " spot lights request shadows, but the renderer supports " +
                std::to_string(caps.spotShadowLimit) + ".");
}

void ValidatePhysics(const Scene& scene, const SceneValidationOptions& options, SceneValidationResult& result)
{
    auto view = scene.View<RigidBodyComponent>();
    for (auto entity : view)
    {
        if (!IsInScope(entity, options))
            continue;
        const auto& body = view.get<RigidBodyComponent>(entity);
        if (!scene.HasAllComponents<WorldTransformComponent>(entity))
            Add(result, SceneValidationSeverity::Error, "PHYSICS_MISSING_WORLD_TRANSFORM", entity, "RigidBodyComponent",
                "Rigid body requires WorldTransformComponent.");
        if (!scene.HasAllComponents<RigidShapeComponent>(entity))
            Add(result, SceneValidationSeverity::Error, "PHYSICS_MISSING_SHAPE", entity, "RigidBodyComponent",
                "Rigid body requires RigidShapeComponent.");
        if (!IsFinite(body.mass) || body.mass < 0.0f)
            Add(result, SceneValidationSeverity::Error, "PHYSICS_INVALID_MASS", entity, "RigidBodyComponent",
                "Rigid body mass must be finite and non-negative.");
        if (body.isAttachedToParent)
        {
            const auto* hierarchy = scene.TryGetComponent<HierarchyComponent>(entity);
            if (!hierarchy || hierarchy->parent == entt::null || !scene.IsValid(hierarchy->parent) ||
                !scene.HasAllComponents<RigidBodyComponent>(hierarchy->parent))
                Add(result, SceneValidationSeverity::Error, "PHYSICS_INVALID_PARENT_ATTACHMENT", entity,
                    "RigidBodyComponent", "Attached rigid body requires a valid rigid-body parent.");
        }
    }
}

void ValidateNavigation(const Scene& scene, const SceneValidationOptions& options, SceneValidationResult& result)
{
    auto navView = scene.View<NavMeshComponent>();
    for (auto entity : navView)
    {
        if (!IsInScope(entity, options))
            continue;
        const auto& nav = navView.get<NavMeshComponent>(entity);
        if (nav.terrainGridResolution < 2)
            Add(result, SceneValidationSeverity::Error, "NAV_INVALID_GRID_RESOLUTION", entity, "NavMeshComponent",
                "Terrain grid resolution must be at least 2.");
        if (!IsFinite(nav.walkableNormalY) || nav.walkableNormalY < -1.0f || nav.walkableNormalY > 1.0f)
            Add(result, SceneValidationSeverity::Error, "NAV_INVALID_WALKABLE_NORMAL", entity, "NavMeshComponent",
                "walkableNormalY must be finite and in [-1, 1].");
        if (!IsFinite(nav.carveHeightPadding) || nav.carveHeightPadding < 0.0f || !IsFinite(nav.carveAgentRadius) ||
            nav.carveAgentRadius < 0.0f)
            Add(result, SceneValidationSeverity::Error, "NAV_INVALID_CARVE_SETTINGS", entity, "NavMeshComponent",
                "NavMesh carve padding and agent radius must be finite and non-negative.");
        if (!nav.needsRebuild && nav.nodes.empty())
            Add(result, SceneValidationSeverity::Warning, "NAV_EMPTY_BAKED_MESH", entity, "NavMeshComponent",
                "NavMesh is marked built but contains no pathfinding nodes.");

        for (size_t i = 0; i < nav.triangles.size(); ++i)
        {
            const auto& triangle = nav.triangles[i];
            for (uint32_t index : triangle.indices)
            {
                if (index >= nav.vertices.size())
                {
                    Add(result, SceneValidationSeverity::Error, "NAV_TRIANGLE_INDEX_OUT_OF_RANGE", entity,
                        "NavMeshComponent", "NavMesh triangle references a vertex outside the vertex array.");
                    break;
                }
            }
        }
        for (size_t i = 0; i < nav.nodes.size(); ++i)
        {
            const auto& node = nav.nodes[i];
            if (node.triangleIndex >= nav.triangles.size())
                Add(result, SceneValidationSeverity::Error, "NAV_NODE_TRIANGLE_OUT_OF_RANGE", entity,
                    "NavMeshComponent", "NavMesh node references an invalid triangle.");
            for (uint32_t neighbor : node.neighbors)
            {
                if (neighbor >= nav.nodes.size())
                {
                    Add(result, SceneValidationSeverity::Error, "NAV_NEIGHBOR_OUT_OF_RANGE", entity, "NavMeshComponent",
                        "NavMesh node references an invalid neighbor.");
                    continue;
                }
                if (options.deepValidation &&
                    std::find(nav.nodes[neighbor].neighbors.begin(), nav.nodes[neighbor].neighbors.end(),
                              static_cast<uint32_t>(i)) == nav.nodes[neighbor].neighbors.end())
                    Add(result, SceneValidationSeverity::Error, "NAV_ASYMMETRIC_CONNECTIVITY", entity,
                        "NavMeshComponent", "NavMesh neighbor connectivity is not bidirectional.");
            }
        }
    }

    auto gridView = scene.View<NavigationGridComponent>();
    for (auto entity : gridView)
    {
        if (IsInScope(entity, options) && !gridView.get<NavigationGridComponent>(entity).IsValid())
            Add(result, SceneValidationSeverity::Error, "NAV_INVALID_GRID", entity, "NavigationGridComponent",
                "Navigation grid dimensions, cell size and cell count are inconsistent.");
    }

    auto followerView = scene.View<PathFollowerComponent>();
    for (auto entity : followerView)
    {
        if (!IsInScope(entity, options))
            continue;
        const auto& follower = followerView.get<PathFollowerComponent>(entity);
        if (!scene.HasAllComponents<PositionComponent, RotationComponent, WorldTransformComponent>(entity))
            Add(result, SceneValidationSeverity::Error, "NAV_FOLLOWER_MISSING_TRANSFORM", entity,
                "PathFollowerComponent", "Path follower requires Position, Rotation and WorldTransform components.");
        if (follower.navigationProviderEntity != entt::null)
        {
            if (!scene.IsValid(follower.navigationProviderEntity))
                Add(result, SceneValidationSeverity::Error, "NAV_INVALID_PROVIDER", entity, "PathFollowerComponent",
                    "Path follower references an invalid navigation provider entity.");
            else if (!scene.HasAllComponents<NavMeshComponent>(follower.navigationProviderEntity) &&
                     !scene.HasAllComponents<NavigationGridComponent>(follower.navigationProviderEntity))
                Add(result, SceneValidationSeverity::Error, "NAV_PROVIDER_MISSING_COMPONENT", entity,
                    "PathFollowerComponent", "Navigation provider entity has neither NavMesh nor NavigationGrid.");
        }
    }
}
}  // namespace

namespace SceneHandlers
{
SceneValidationResult SceneValidator::Validate(const Scene& scene, const SceneValidationOptions& options)
{
    SceneValidationResult result;
    ValidateHierarchy(scene, options, result);
    ValidateCameras(scene, options, result);
    ValidateLights(scene, options, result);
    ValidatePhysics(scene, options, result);
    ValidateNavigation(scene, options, result);
    return result;
}

void SceneValidator::LogIssues(const SceneValidationResult& result)
{
    for (const auto& issue : result.issues)
    {
        const std::string entityText =
            issue.entity == entt::null ? std::string("scene") : std::to_string(static_cast<uint32_t>(issue.entity));
        const std::string text = issue.code + " [" + entityText + "] " + issue.component + ": " + issue.message;
        switch (issue.severity)
        {
            case SceneValidationSeverity::Info:
                LOGGER_INFO("SceneValidator") << text;
                break;
            case SceneValidationSeverity::Warning:
                LOGGER_WARN("SceneValidator") << text;
                break;
            case SceneValidationSeverity::Error:
            case SceneValidationSeverity::Fatal:
                LOGGER_ERROR("SceneValidator") << text;
                break;
        }
    }
}
}  // namespace SceneHandlers
