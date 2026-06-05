#include <navigation/logic/navigation_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <navigation/logic/navmesh_generator.h>
#include <navigation/logic/pathfinding.h>
#include <physics/interface/i_physics_world.h>
#include <resource/logic/resource_manager.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

REGISTER_SYSTEM(NavigationSystem)

namespace
{
std::vector<glm::vec3> SmoothPathCorners(const std::vector<glm::vec3>& path, int iterations)
{
    if (path.size() < 3 || iterations <= 0)
        return path;

    std::vector<glm::vec3> result = path;
    for (int it = 0; it < iterations; ++it)
    {
        std::vector<glm::vec3> smoothed;
        smoothed.reserve(result.size() * 2);
        smoothed.push_back(result.front());

        for (size_t i = 1; i + 1 < result.size(); ++i)
        {
            const glm::vec3& prev = result[i - 1];
            const glm::vec3& curr = result[i];
            const glm::vec3& next = result[i + 1];
            smoothed.push_back(glm::mix(curr, prev, 0.22f));
            smoothed.push_back(glm::mix(curr, next, 0.22f));
        }

        smoothed.push_back(result.back());
        result = std::move(smoothed);
    }
    return result;
}

bool HasTag(const std::vector<std::string>& tags, const std::string& value)
{
    return std::find(tags.begin(), tags.end(), value) != tags.end();
}

glm::vec3 ComputeLocalAvoidance(Scene& scene, entt::entity self, const glm::vec3& position,
                                const glm::vec3& moveDir, const PathFollowerComponent& follower,
                                IPhysicsWorld* physics, const std::vector<std::string>& obstacleTags, bool fly3D)
{
    if (!follower.localAvoidanceEnabled)
        return glm::vec3(0.0f);

    glm::vec3 avoidance(0.0f);
    auto view = scene.registry.view<PositionComponent, PathFollowerComponent, InfoComponent>();
    for (auto other : view)
    {
        if (other == self)
            continue;

        const auto& otherInfo = view.get<InfoComponent>(other);
        if (!otherInfo.isActive)
            continue;

        const auto& otherFollower = view.get<PathFollowerComponent>(other);
        if (!otherFollower.isMoving && !otherFollower.pathPending)
            continue;

        glm::vec3 offset = position - view.get<PositionComponent>(other).value;
        if (!fly3D)
            offset.y = 0.0f;

        const float distance = glm::length(offset);
        if (distance <= 0.0001f || distance >= follower.separationRadius)
            continue;

        const float strength = 1.0f - (distance / follower.separationRadius);
        avoidance += (offset / distance) * strength * follower.separationWeight;
    }

    if (physics && follower.obstacleAvoidanceDistance > 0.0f && glm::length(moveDir) > 0.001f)
    {
        const glm::vec3 rayOrigin = position + glm::vec3(0.0f, 0.5f, 0.0f);
        auto hit = physics->Raycast(rayOrigin, glm::normalize(moveDir), follower.obstacleAvoidanceDistance, self);
        if (hit.hasHit && scene.registry.valid(hit.entity))
        {
            const auto* info = scene.registry.try_get<InfoComponent>(hit.entity);
            if (info && HasTag(obstacleTags, info->tag))
            {
                glm::vec3 side = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), moveDir);
                if (glm::length(side) < 0.001f)
                    side = glm::vec3(1.0f, 0.0f, 0.0f);
                else
                    side = glm::normalize(side);

                const float proximity = 1.0f - glm::clamp(hit.distance / follower.obstacleAvoidanceDistance, 0.0f, 1.0f);
                avoidance += side * proximity * follower.obstacleAvoidanceWeight;
            }
        }
    }

    if (!fly3D)
        avoidance.y = 0.0f;
    return avoidance;
}
}  // namespace

void NavigationSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<NavigationSystem>(this);
}

void NavigationSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    UpdateNavMesh(scene);
    UpdatePathFollowing(scene, dt);
}

void NavigationSystem::UpdateNavMesh(Scene& scene)
{
    auto& sl = ServiceLocator::Instance();
    auto& resources = sl.Require<ResourceManager>();

    auto view = scene.registry.view<NavMeshComponent>();
    for (auto entity : view)
    {
        auto& navMesh = view.get<NavMeshComponent>(entity);
        if (navMesh.needsRebuild)
        {
            LOGGER_INFO("NavigationSystem") << "NavMesh rebuild triggered for entity " << (uint32_t)entity;
            NavMeshGenerator::Generate(scene, navMesh, &resources, m_WalkableTags, m_CarveTags);
            LOGGER_INFO("NavigationSystem")
                << "NavMesh state: Nodes=" << navMesh.nodes.size() << ", Tris=" << navMesh.triangles.size();
            if (navMesh.nodes.empty())
            {
                navMesh.needsRebuild = true;
            }
        }
    }
}

void NavigationSystem::UpdatePathFollowing(Scene& scene, float dt)
{
    auto& sl = ServiceLocator::Instance();
    auto physics_ptr = sl.Resolve<IPhysicsWorld>();

    auto view =
        scene.registry.view<PositionComponent, RotationComponent, WorldTransformComponent, PathFollowerComponent>();

    NavMeshComponent* globalNavMesh = nullptr;
    auto navMeshView = scene.registry.view<NavMeshComponent>();
    if (!navMeshView.empty())
    {
        globalNavMesh = &navMeshView.get<NavMeshComponent>(navMeshView.front());
    }

    NavigationGridComponent* globalGrid = nullptr;
    auto gridView = scene.registry.view<NavigationGridComponent>();
    for (auto gridEntity : gridView)
    {
        auto& grid = gridView.get<NavigationGridComponent>(gridEntity);
        if (grid.IsValid())
        {
            globalGrid = &grid;
            break;
        }
    }

    for (auto entity : view)
    {
        auto& pos = view.get<PositionComponent>(entity);
        auto& rot = view.get<RotationComponent>(entity);
        auto& world = view.get<WorldTransformComponent>(entity);
        auto& follower = view.get<PathFollowerComponent>(entity);
        NavMeshComponent* selectedNavMesh = globalNavMesh;
        NavigationGridComponent* selectedGrid = globalGrid;

        if (follower.navigationProviderEntity != entt::null && scene.registry.valid(follower.navigationProviderEntity))
        {
            if (follower.pathfindingOptions.provider == NavigationProvider::NavMesh ||
                follower.pathfindingOptions.provider == NavigationProvider::Auto)
            {
                if (auto* nav = scene.registry.try_get<NavMeshComponent>(follower.navigationProviderEntity))
                    selectedNavMesh = nav;
            }
            if (follower.pathfindingOptions.provider == NavigationProvider::Grid ||
                follower.pathfindingOptions.provider == NavigationProvider::Auto)
            {
                if (auto* grid = scene.registry.try_get<NavigationGridComponent>(follower.navigationProviderEntity);
                    grid && grid->IsValid())
                {
                    selectedGrid = grid;
                }
            }
        }

        if (follower.pathPending)
        {
            if (follower.pathfindingOptions.criteria == PathfindingCriteria::StraightLine)
            {
                follower.currentPath = {pos.value, follower.targetPosition};
                follower.currentPathIndex = 0;
                follower.pathPending = false;
                follower.isMoving = true;
                follower.debugPlannedPath = follower.currentPath;
                follower.debugTraveledPath.clear();
                if (follower.recordDebugPath)
                    follower.debugTraveledPath.push_back(pos.value);
            }
            else if (selectedGrid &&
                     (follower.pathfindingOptions.provider == NavigationProvider::Grid ||
                      (follower.pathfindingOptions.provider == NavigationProvider::Auto &&
                       (!selectedNavMesh || selectedNavMesh->nodes.empty()))))
            {
                follower.currentPath =
                    Pathfinding::FindGridPath(pos.value, follower.targetPosition, *selectedGrid,
                                              follower.pathfindingOptions);
                follower.currentPathIndex = 0;
                follower.pathPending = false;
                follower.isMoving = !follower.currentPath.empty();
                follower.debugTraveledPath.clear();
                if (follower.recordDebugPath)
                    follower.debugTraveledPath.push_back(pos.value);
                follower.debugPlannedPath = follower.currentPath;
            }
            else if (selectedNavMesh && !selectedNavMesh->nodes.empty())
            {
                LOGGER_INFO("NavigationSystem") << "Pathfinding request for entity " << (uint32_t)entity << ": Start=("
                                                << pos.value.x << "," << pos.value.y << "," << pos.value.z << ")"
                                                << " Target=(" << follower.targetPosition.x << ","
                                                << follower.targetPosition.y << "," << follower.targetPosition.z << ")"
                                                << " NavMeshNodes=" << selectedNavMesh->nodes.size()
                                                << " Strategy=" << (int)follower.pathfindingOptions.criteria;

                follower.currentPath = Pathfinding::FindPath(pos.value, follower.targetPosition, *selectedNavMesh,
                                                             follower.pathfindingOptions);
                follower.currentPathIndex = 0;
                follower.pathPending = false;
                follower.isMoving = !follower.currentPath.empty();
                follower.debugTraveledPath.clear();
                if (follower.recordDebugPath)
                    follower.debugTraveledPath.push_back(pos.value);

                if (follower.isMoving && follower.currentPath.size() > 2 && physics_ptr &&
                    follower.pathfindingOptions.criteria == PathfindingCriteria::Smoothest)
                {
                    std::vector<glm::vec3> smoothedPath;
                    smoothedPath.push_back(follower.currentPath[0]);

                    size_t curr = 0;
                    while (curr < follower.currentPath.size() - 1)
                    {
                        size_t nextFound = curr + 1;
                        for (size_t test = follower.currentPath.size() - 1; test > curr + 1; --test)
                        {
                            glm::vec3 start = follower.currentPath[curr] + glm::vec3(0, 0.5f, 0);
                            glm::vec3 end = follower.currentPath[test] + glm::vec3(0, 0.5f, 0);
                            glm::vec3 dir = end - start;
                            float dist = glm::length(dir);
                            if (dist > 0.001f)
                            {
                                auto hit = physics_ptr->Raycast(start, glm::normalize(dir), dist);
                                if (!hit.hasHit)
                                {
                                    nextFound = test;
                                    break;
                                }
                            }
                        }
                        smoothedPath.push_back(follower.currentPath[nextFound]);
                        curr = nextFound;
                    }
                    follower.currentPath = smoothedPath;
                }
                if (follower.isMoving && follower.pathfindingOptions.criteria == PathfindingCriteria::Smoothest)
                    follower.currentPath = SmoothPathCorners(follower.currentPath, 2);
                follower.debugPlannedPath = follower.currentPath;

                LOGGER_INFO("NavigationSystem") << "Pathfinding Result: " << (follower.isMoving ? "SUCCESS" : "FAILED")
                                                << " PathSize=" << follower.currentPath.size();
            }
            else
            {
                if (follower.pathPending)
                {
                    LOGGER_WARN("NavigationSystem") << "Path pending but no usable navigation provider is available!";
                    follower.pathPending = false;
                    follower.isMoving = false;
                    follower.currentPath.clear();
                }
            }
        }

        if (follower.isMoving && !follower.currentPath.empty())
        {
            const bool fly3D = follower.pathfindingOptions.criteria == PathfindingCriteria::Shortest ||
                               follower.pathfindingOptions.criteria == PathfindingCriteria::Smoothest ||
                               follower.pathfindingOptions.criteria == PathfindingCriteria::HighGround;
            glm::vec3 target = follower.currentPath[follower.currentPathIndex];
            glm::vec3 diff = target - pos.value;
            float distance = fly3D ? glm::length(diff) : glm::length(glm::vec3(diff.x, 0.0f, diff.z));

            if (distance < follower.arrivalDistance)
            {
                follower.currentPathIndex++;
                if (follower.currentPathIndex >= follower.currentPath.size())
                {
                    follower.isMoving = false;
                    follower.currentPath.clear();
                }
            }
            else
            {
                glm::vec3 moveDir = fly3D ? diff : glm::vec3(diff.x, 0.0f, diff.z);
                float moveDist = glm::length(moveDir);
                if (moveDist > 0.001f)
                {
                    moveDir /= moveDist;
                    glm::vec3 avoidance =
                        ComputeLocalAvoidance(scene, entity, pos.value, moveDir, follower, physics_ptr, m_CarveTags,
                                              fly3D);
                    if (glm::length(avoidance) > 0.001f)
                    {
                        glm::vec3 adjustedDir = moveDir + avoidance;
                        if (glm::length(adjustedDir) > 0.001f)
                            moveDir = glm::normalize(adjustedDir);
                    }
                    glm::vec3 moveStep = moveDir * follower.moveSpeed * dt;
                    if (follower.lockMoveX)
                        moveStep.x = 0.0f;
                    if (follower.lockMoveY)
                        moveStep.y = 0.0f;
                    if (follower.lockMoveZ)
                        moveStep.z = 0.0f;
                    pos.value += moveStep;
                    if (follower.recordDebugPath &&
                        (follower.debugTraveledPath.empty() ||
                         glm::distance(follower.debugTraveledPath.back(), pos.value) > 0.35f))
                    {
                        follower.debugTraveledPath.push_back(pos.value);
                    }
                    world.isDirty = true;
                }
                else
                {
                    follower.currentPathIndex++;
                    continue;
                }

                if (glm::length(moveDir) > 0.001f)
                {
                    glm::vec3 groundNormal(0, 1, 0);
                    if (physics_ptr && !fly3D)
                    {
                        auto groundHit = physics_ptr->Raycast(pos.value + glm::vec3(0, 100.0f, 0), glm::vec3(0, -1, 0),
                                                              200.0f, entity);

                        if (groundHit.hasHit)
                        {
                            bool isObstacle = false;
                            if (scene.registry.valid(groundHit.entity))
                            {
                                auto* info = scene.registry.try_get<InfoComponent>(groundHit.entity);
                                if (info && info->tag == "obstacle")
                                {
                                    isObstacle = true;
                                }
                            }
                            if (!isObstacle)
                            {
                                groundNormal = groundHit.hitNormal;
                                if (!glm::any(glm::isnan(groundHit.hitPoint)) &&
                                    std::abs(groundHit.hitPoint.y) < 10000.0f)
                                {
                                    pos.value.y = groundHit.hitPoint.y;
                                }
                            }
                        }
                    }

                    glm::vec3 crossDir = glm::cross(moveDir, groundNormal);
                    if (glm::length(crossDir) > 0.001f)
                    {
                        glm::vec3 right = glm::normalize(crossDir);
                        glm::vec3 up = groundNormal;
                        glm::vec3 forward = glm::cross(up, right);

                        if (follower.lockXPitch || follower.lockZRoll)
                        {
                            glm::vec3 worldUp(0, 1, 0);
                            glm::vec3 moveDirXZ(0.0f);
                            float moveLenXZ = glm::length(glm::vec3(moveDir.x, 0, moveDir.z));
                            if (moveLenXZ > 0.001f)
                            {
                                moveDirXZ = glm::vec3(moveDir.x, 0, moveDir.z) / moveLenXZ;
                            }
                            else
                            {
                                moveDirXZ = glm::vec3(0, 0, 1);
                            }

                            if (follower.lockXPitch && follower.lockZRoll)
                            {
                                up = worldUp;
                                glm::vec3 crossXZ = glm::cross(moveDirXZ, worldUp);
                                if (glm::length(crossXZ) > 0.001f)
                                {
                                    right = glm::normalize(crossXZ);
                                }
                                else
                                {
                                    right = glm::vec3(1, 0, 0);
                                }
                                forward = moveDirXZ;
                            }
                            else if (follower.lockXPitch)
                            {
                                forward = moveDirXZ;
                                up = glm::normalize(glm::cross(right, forward));
                            }
                            else if (follower.lockZRoll)
                            {
                                glm::vec3 crossXZ = glm::cross(forward, worldUp);
                                if (glm::length(crossXZ) > 0.001f)
                                {
                                    right = glm::normalize(crossXZ);
                                }
                                else
                                {
                                    right = glm::vec3(1, 0, 0);
                                }
                                up = glm::normalize(glm::cross(right, forward));
                            }
                        }

                        if (glm::length(forward) > 0.001f)
                        {
                            glm::mat3 rotMat;
                            rotMat[0] = right;
                            rotMat[1] = up;
                            rotMat[2] = -forward;
                            glm::quat targetRot = glm::quat_cast(rotMat);

                            if (glm::length(follower.rotationOffset) > 0.001f)
                            {
                                glm::quat offsetQuat = glm::quat(glm::radians(follower.rotationOffset));
                                targetRot = targetRot * offsetQuat;
                            }

                            if (follower.lockXPitch || follower.lockYYaw || follower.lockZRoll)
                            {
                                glm::vec3 currentEuler = glm::eulerAngles(rot.value);
                                glm::vec3 targetEuler = glm::eulerAngles(targetRot);
                                if (follower.lockXPitch)
                                    targetEuler.x = currentEuler.x;
                                if (follower.lockYYaw)
                                    targetEuler.y = currentEuler.y;
                                if (follower.lockZRoll)
                                    targetEuler.z = currentEuler.z;
                                targetRot = glm::quat(targetEuler);
                            }

                            float dot = 0.0f;
                            float angleDiff = 0.0f;

                            if (glm::length(rot.value) > 0.001f && glm::length(targetRot) > 0.001f)
                            {
                                dot = glm::dot(glm::normalize(rot.value), glm::normalize(targetRot));
                                angleDiff = glm::acos(glm::min(glm::abs(dot), 1.0f)) * 2.0f;
                            }

                            if (angleDiff > 0.001f && !glm::isnan(angleDiff))
                            {
                                follower.currentRotationVelocity += follower.rotationAcceleration * dt;
                                if (follower.currentRotationVelocity > follower.maxRotationSpeed)
                                {
                                    follower.currentRotationVelocity = follower.maxRotationSpeed;
                                }

                                float step = (follower.currentRotationVelocity * dt) / angleDiff;
                                if (step > 1.0f)
                                    step = 1.0f;
                                if (!glm::isnan(step) && step > 0.0f)
                                {
                                    rot.value = glm::slerp(rot.value, targetRot, step);
                                }
                            }
                            else
                            {
                                follower.currentRotationVelocity = 0.0f;
                                if (!std::isnan(targetRot.x) && !std::isnan(targetRot.y) && !std::isnan(targetRot.z) &&
                                    !std::isnan(targetRot.w) && glm::length(targetRot) > 0.001f)
                                {
                                    rot.value = targetRot;
                                }
                            }
                        }
                    }
                }

                world.isDirty = true;
            }
        }
    }
}

void NavigationSystem::StopMoving(Scene& scene, entt::entity entity)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->isMoving = false;
        follower->pathPending = false;
        follower->currentPath.clear();
        follower->debugPlannedPath.clear();
        follower->debugTraveledPath.clear();
        follower->currentPathIndex = 0;
    }
}

bool NavigationSystem::IsMoving(Scene& scene, entt::entity entity)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    return follower ? follower->isMoving : false;
}

void NavigationSystem::SetMoveSpeed(Scene& scene, entt::entity entity, float speed)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
        follower->moveSpeed = speed;
}

float NavigationSystem::GetRemainingDistance(Scene& scene, entt::entity entity)
{
    auto* pos = scene.registry.try_get<PositionComponent>(entity);
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);

    if (!pos || !follower || !follower->isMoving || follower->currentPath.empty())
        return 0.0f;

    float dist = 0.0f;
    glm::vec3 lastPoint = pos->value;

    for (size_t i = follower->currentPathIndex; i < follower->currentPath.size(); ++i)
    {
        dist += glm::distance(lastPoint, follower->currentPath[i]);
        lastPoint = follower->currentPath[i];
    }

    return dist;
}

void NavigationSystem::MoveTo(Scene& scene, entt::entity entity, const glm::vec3& position)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->targetPosition = position;
        follower->pathPending = true;
    }
}

bool NavigationSystem::HasTarget(Scene& scene, entt::entity entity)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    return follower ? (follower->pathPending || follower->isMoving) : false;
}

void NavigationSystem::Render(Scene& scene)
{
    if (!m_ShowDebug)
        return;

    auto physics_ptr = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    if (!physics_ptr)
        return;

    auto navMeshView = scene.registry.view<NavMeshComponent>();
    for (auto entity : navMeshView)
    {
        auto& navMesh = navMeshView.get<NavMeshComponent>(entity);

        for (const auto& tri : navMesh.triangles)
        {
            constexpr float offset = 0.02f;
            glm::vec3 v0 = navMesh.vertices[tri.indices[0]] + glm::vec3(0.0f, offset, 0.0f);
            glm::vec3 v1 = navMesh.vertices[tri.indices[1]] + glm::vec3(0.0f, offset, 0.0f);
            glm::vec3 v2 = navMesh.vertices[tri.indices[2]] + glm::vec3(0.0f, offset, 0.0f);

            physics_ptr->DrawLine(v0, v1, glm::vec3(0, 0, 1));
            physics_ptr->DrawLine(v1, v2, glm::vec3(0, 0, 1));
            physics_ptr->DrawLine(v2, v0, glm::vec3(0, 0, 1));
        }
    }

    auto pathView = scene.registry.view<PositionComponent, PathFollowerComponent>();
    for (auto entity : pathView)
    {
        auto& follower = pathView.get<PathFollowerComponent>(entity);
        const auto& plannedPath = !follower.debugPlannedPath.empty() ? follower.debugPlannedPath : follower.currentPath;
        if (!plannedPath.empty())
        {
            for (size_t i = 1; i < plannedPath.size(); ++i)
            {
                glm::vec3 p0 = plannedPath[i - 1] + glm::vec3(0.0f, 0.35f, 0.0f);
                glm::vec3 p1 = plannedPath[i] + glm::vec3(0.0f, 0.35f, 0.0f);

                physics_ptr->DrawLine(p0, p1, glm::vec3(0.0f, 1.0f, 0.15f));
            }
        }

        if (follower.debugTraveledPath.size() > 1)
        {
            for (size_t i = 1; i < follower.debugTraveledPath.size(); ++i)
            {
                glm::vec3 p0 = follower.debugTraveledPath[i - 1] + glm::vec3(0.0f, 0.65f, 0.0f);
                glm::vec3 p1 = follower.debugTraveledPath[i] + glm::vec3(0.0f, 0.65f, 0.0f);
                physics_ptr->DrawLine(p0, p1, glm::vec3(1.0f, 0.9f, 0.05f));
            }
        }
    }
}

void NavigationSystem::AddWalkableTag(const std::string& tag)
{
    if (std::find(m_WalkableTags.begin(), m_WalkableTags.end(), tag) == m_WalkableTags.end())
    {
        m_WalkableTags.push_back(tag);
    }
}

void NavigationSystem::ClearWalkableTags()
{
    m_WalkableTags.clear();
    m_WalkableTags.push_back("walkable");
}

void NavigationSystem::AddCarveTag(const std::string& tag)
{
    if (std::find(m_CarveTags.begin(), m_CarveTags.end(), tag) == m_CarveTags.end())
    {
        m_CarveTags.push_back(tag);
    }
}

void NavigationSystem::ClearCarveTags()
{
    m_CarveTags.clear();
    m_CarveTags.push_back("obstacle");
}

void NavigationSystem::SetPathfindingCriteria(Scene& scene, entt::entity entity, PathfindingCriteria criteria)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
        follower->pathfindingOptions.criteria = criteria;
}

void NavigationSystem::SetPreferredTags(Scene& scene, entt::entity entity, const std::vector<std::string>& tags)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
        follower->pathfindingOptions.preferredTags = tags;
}

void NavigationSystem::SetCustomCostFunction(Scene& scene, entt::entity entity,
                                             std::function<float(uint32_t, uint32_t, const NavMeshComponent&)> func)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->pathfindingOptions.criteria = PathfindingCriteria::Custom;
        follower->pathfindingOptions.customCostFunc = func;
    }
}

void NavigationSystem::SetCustomGridCostFunction(
    Scene& scene, entt::entity entity, std::function<float(uint32_t, uint32_t, const NavigationGridComponent&)> func)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->pathfindingOptions.criteria = PathfindingCriteria::Custom;
        follower->pathfindingOptions.customGridCostFunc = func;
    }
}

void NavigationSystem::SetNavigationProviderEntity(Scene& scene, entt::entity entity, entt::entity provider,
                                                   NavigationProvider providerType)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->navigationProviderEntity = provider;
        if (providerType != NavigationProvider::Auto)
            follower->pathfindingOptions.provider = providerType;
    }
}

std::vector<entt::id_type> NavigationSystem::GetReadComponents() const
{
    return {entt::type_id<PositionComponent>().hash(), entt::type_id<NavMeshComponent>().hash(),
            entt::type_id<NavigationGridComponent>().hash()};
}

std::vector<entt::id_type> NavigationSystem::GetWriteComponents() const
{
    return {entt::type_id<PositionComponent>().hash(), entt::type_id<RotationComponent>().hash(),
            entt::type_id<PathFollowerComponent>().hash()};
}
