#include <navigation/logic/navigation_system.h>
#include <core/logic/logger.h>
#include <core/logic/job_system.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <navigation/logic/navmesh_generator.h>
#include <navigation/logic/pathfinding.h>
#include <physics/interface/i_physics_world.h>
#include <resource/logic/resource_manager.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <chrono>


namespace
{
struct AgentCell
{
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const AgentCell&) const = default;
};

struct AgentCellHash
{
    size_t operator()(const AgentCell& cell) const
    {
        size_t seed = std::hash<int>{}(cell.x);
        seed ^= std::hash<int>{}(cell.y) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(cell.z) + 0x9e3779b9u + (seed << 6) + (seed >> 2);
        return seed;
    }
};

using AgentGrid = std::unordered_map<AgentCell, std::vector<entt::entity>, AgentCellHash>;

AgentCell ToAgentCell(const glm::vec3& position, float cellSize, bool fly3D)
{
    return {static_cast<int>(std::floor(position.x / cellSize)),
            fly3D ? static_cast<int>(std::floor(position.y / cellSize)) : 0,
            static_cast<int>(std::floor(position.z / cellSize))};
}

bool UsesThreeDimensionalMovement(const PathFollowerComponent& follower)
{
    return follower.pathfindingOptions.criteria == PathfindingCriteria::Shortest ||
           follower.pathfindingOptions.criteria == PathfindingCriteria::Smoothest ||
           follower.pathfindingOptions.criteria == PathfindingCriteria::HighGround;
}

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
                                IPhysicsWorld* physics, const std::vector<std::string>& obstacleTags, bool fly3D,
                                const AgentGrid* agents, float cellSize)
{
    if (!follower.localAvoidanceEnabled)
        return glm::vec3(0.0f);

    glm::vec3 avoidance(0.0f);
    auto accumulateAgent = [&](entt::entity other) {
        if (other == self)
            return;
        const auto* otherInfo = scene.TryGetComponent<InfoComponent>(other);
        const auto* otherFollower = scene.TryGetComponent<PathFollowerComponent>(other);
        const auto* otherPosition = scene.TryGetComponent<PositionComponent>(other);
        if (!otherInfo || !otherInfo->isActive || !otherFollower || !otherPosition ||
            (!otherFollower->isMoving && !otherFollower->pathPending))
            return;

        glm::vec3 offset = position - otherPosition->value;
        if (!fly3D)
            offset.y = 0.0f;
        const float distance = glm::length(offset);
        if (distance <= 0.0001f || distance >= follower.separationRadius)
            return;
        const float strength = 1.0f - (distance / follower.separationRadius);
        avoidance += (offset / distance) * strength * follower.separationWeight;
    };

    if (agents)
    {
        const AgentCell center = ToAgentCell(position, cellSize, fly3D);
        const int cellRadius = (std::max)(1, static_cast<int>(std::ceil(follower.separationRadius / cellSize)));
        const int minY = fly3D ? -cellRadius : 0;
        const int maxY = fly3D ? cellRadius : 0;
        for (int dz = -cellRadius; dz <= cellRadius; ++dz)
        for (int dy = minY; dy <= maxY; ++dy)
        for (int dx = -cellRadius; dx <= cellRadius; ++dx)
        {
            const auto cell = agents->find({center.x + dx, center.y + dy, center.z + dz});
            if (cell == agents->end())
                continue;
            for (const entt::entity other : cell->second)
                accumulateAgent(other);
        }
    }
    else
    {
        auto allAgents = scene.View<PositionComponent, PathFollowerComponent, InfoComponent>();
        for (const entt::entity other : allAgents)
            accumulateAgent(other);
    }

    if (physics && follower.obstacleAvoidanceDistance > 0.0f && glm::length(moveDir) > 0.001f)
    {
        const glm::vec3 rayOrigin = position + glm::vec3(0.0f, 0.5f, 0.0f);
        auto hit = physics->Raycast(rayOrigin, glm::normalize(moveDir), follower.obstacleAvoidanceDistance, self);
        if (hit.hasHit && scene.IsValid(hit.entity))
        {
            const auto* info = scene.TryGetComponent<InfoComponent>(hit.entity);
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

bool BoundsNearlyEqual(const glm::vec3& leftMin, const glm::vec3& leftMax,
                       const glm::vec3& rightMin, const glm::vec3& rightMax)
{
    constexpr float epsilon = 0.001f;
    return glm::all(glm::lessThanEqual(glm::abs(leftMin - rightMin), glm::vec3(epsilon))) &&
           glm::all(glm::lessThanEqual(glm::abs(leftMax - rightMax), glm::vec3(epsilon)));
}
}  // namespace

void NavigationSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<NavigationSystem>(this);
    sl.Register<INavigationService>(this);
}

void NavigationSystem::Shutdown()
{
    // Jobs own immutable copies only, so dropping futures safely cancels
    // delivery without allowing workers to touch a destroyed scene/system.
    m_PendingPaths.clear();
    m_NavMeshRuntime.clear();
    m_RuntimeScene = nullptr;
}

void NavigationSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    SetSpatialHashConfig(config.navigationSpatialHashEnabled, config.navigationAgentCellSize);
    SetAsyncPathfindingConfig(config.navigationAsyncPathfindingEnabled, config.navigationMaxPathRequestsPerFrame);
    m_NavMeshRebuildBudgetEnabled = config.navMeshRebuildBudgetEnabled;
    m_MaxNavMeshRebuildsPerFrame = (std::max)(1, config.maxNavMeshRebuildsPerFrame);
    m_NavMeshDirtyTilesEnabled = config.navigationDirtyTilesEnabled;
    m_NavMeshTileSize = (std::max)(0.25f, config.navigationNavMeshTileSize);
    m_MaxNavMeshDirtyTilesPerFrame = (std::max)(1, config.navigationMaxDirtyTilesPerFrame);
}

void NavigationSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    UpdateNavMesh(scene, dt);
    UpdatePathFollowing(scene, dt);
}

void NavigationSystem::UpdateNavMesh(Scene& scene, float dt)
{
    if (m_RuntimeScene != &scene)
    {
        m_NavMeshRuntime.clear();
        m_RuntimeScene = &scene;
    }
    auto& sl = ServiceLocator::Instance();
    auto& resources = sl.Require<ResourceManager>();

    auto view = scene.View<NavMeshComponent>();
    std::erase_if(m_NavMeshRuntime, [&](const auto& entry) {
        return !scene.IsValid(entry.first) || !scene.HasAllComponents<NavMeshComponent>(entry.first);
    });
    int rebuilds = 0;
    for (auto entity : view)
    {
        auto& navMesh = view.get<NavMeshComponent>(entity);
        auto& runtime = m_NavMeshRuntime[entity];
        navMesh.rebuildRetryRemaining = (std::max)(0.0f, navMesh.rebuildRetryRemaining - dt);
        if (navMesh.rebuildRetryRemaining > 0.0f)
            continue;
        if (navMesh.needsRebuild)
        {
            if (m_NavMeshRebuildBudgetEnabled && rebuilds >= m_MaxNavMeshRebuildsPerFrame)
                continue;
            ++rebuilds;
            LOGGER_INFO("NavigationSystem") << "NavMesh rebuild triggered for entity " << (uint32_t)entity;
            NavMeshGenerator::Generate(scene, navMesh, &resources, m_WalkableTags, m_CarveTags,
                                       &runtime.uncarvedTriangles);
            runtime.dirtyTiles.clear();
            runtime.obstacles.clear();
            for (const auto& obstacle : NavMeshGenerator::CollectObstacleBounds(scene, navMesh, m_CarveTags))
                runtime.obstacles.emplace(obstacle.entity, DirtyBounds{obstacle.min, obstacle.max});
            LOGGER_INFO("NavigationSystem")
                << "NavMesh state: Nodes=" << navMesh.nodes.size() << ", Tris=" << navMesh.triangles.size();
            if (navMesh.nodes.empty())
            {
                navMesh.needsRebuild = true;
                navMesh.rebuildRetryRemaining = 1.0f;
            }
            continue;
        }

        if (!navMesh.isDynamic)
            continue;

        const auto obstacles = NavMeshGenerator::CollectObstacleBounds(scene, navMesh, m_CarveTags);
        std::unordered_map<entt::entity, DirtyBounds> currentObstacles;
        currentObstacles.reserve(obstacles.size());
        bool obstacleChanged = false;
        for (const auto& obstacle : obstacles)
        {
            const DirtyBounds current{obstacle.min, obstacle.max};
            currentObstacles.emplace(obstacle.entity, current);
            const auto previous = runtime.obstacles.find(obstacle.entity);
            if (previous == runtime.obstacles.end())
            {
                obstacleChanged = true;
                QueueDirtyBounds(runtime, current);
            }
            else if (!BoundsNearlyEqual(previous->second.min, previous->second.max, current.min, current.max))
            {
                obstacleChanged = true;
                QueueDirtyBounds(runtime,
                                 {glm::min(previous->second.min, current.min), glm::max(previous->second.max, current.max)});
            }
        }
        for (const auto& [obstacleEntity, previous] : runtime.obstacles)
        {
            if (!currentObstacles.contains(obstacleEntity))
            {
                obstacleChanged = true;
                QueueDirtyBounds(runtime, previous);
            }
        }
        runtime.obstacles = std::move(currentObstacles);

        if (obstacleChanged && (!m_NavMeshDirtyTilesEnabled || runtime.uncarvedTriangles.empty()))
        {
            navMesh.needsRebuild = true;
            runtime.dirtyTiles.clear();
            continue;
        }

        if (!runtime.dirtyTiles.empty() &&
            (!m_NavMeshRebuildBudgetEnabled || rebuilds < m_MaxNavMeshRebuildsPerFrame))
        {
            DirtyBounds merged = runtime.dirtyTiles.front();
            runtime.dirtyTiles.pop_front();
            int tiles = 1;
            while (!runtime.dirtyTiles.empty() && tiles < m_MaxNavMeshDirtyTilesPerFrame)
            {
                merged.min = glm::min(merged.min, runtime.dirtyTiles.front().min);
                merged.max = glm::max(merged.max, runtime.dirtyTiles.front().max);
                runtime.dirtyTiles.pop_front();
                ++tiles;
            }
            NavMeshGenerator::RebuildRegion(scene, navMesh, runtime.uncarvedTriangles, merged.min, merged.max,
                                            m_CarveTags);
            ++rebuilds;
        }
    }
}

bool NavigationSystem::QueueDirtyBounds(NavMeshRuntime& runtime, const DirtyBounds& bounds)
{
    const auto finite = [](const glm::vec3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!finite(bounds.min) || !finite(bounds.max))
        return false;
    const glm::vec3 minimum = glm::min(bounds.min, bounds.max);
    const glm::vec3 maximum = glm::max(bounds.min, bounds.max);
    const int minX = static_cast<int>(std::floor(minimum.x / m_NavMeshTileSize));
    const int maxX = static_cast<int>(std::floor(maximum.x / m_NavMeshTileSize));
    const int minZ = static_cast<int>(std::floor(minimum.z / m_NavMeshTileSize));
    const int maxZ = static_cast<int>(std::floor(maximum.z / m_NavMeshTileSize));
    const int64_t tileCount = static_cast<int64_t>(maxX - minX + 1) * (maxZ - minZ + 1);
    if (tileCount <= 0)
        return false;
    if (tileCount > 4096)
    {
        runtime.dirtyTiles.push_back({minimum, maximum});
        return true;
    }

    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const DirtyBounds tile{{x * m_NavMeshTileSize, minimum.y, z * m_NavMeshTileSize},
                                   {(x + 1) * m_NavMeshTileSize, maximum.y,
                                    (z + 1) * m_NavMeshTileSize}};
            const bool queued = std::any_of(runtime.dirtyTiles.begin(), runtime.dirtyTiles.end(),
                                            [&](const DirtyBounds& existing) {
                                                return existing.min.x == tile.min.x && existing.min.z == tile.min.z;
                                            });
            if (!queued)
                runtime.dirtyTiles.push_back(tile);
        }
    }
    return true;
}

void NavigationSystem::UpdatePathFollowing(Scene& scene, float dt)
{
    auto& sl = ServiceLocator::Instance();
    auto physics_ptr = sl.Resolve<IPhysicsWorld>();
    for (auto pending = m_PendingPaths.begin(); pending != m_PendingPaths.end();)
    {
        if (pending->second.result.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            ++pending;
            continue;
        }

        std::vector<glm::vec3> path = pending->second.result.get();
        if (scene.IsValid(pending->first))
        {
            const bool navMeshStillCurrent =
                pending->second.navMeshProvider == entt::null ||
                (scene.IsValid(pending->second.navMeshProvider) &&
                 scene.HasAllComponents<NavMeshComponent>(pending->second.navMeshProvider) &&
                 scene.GetComponent<NavMeshComponent>(pending->second.navMeshProvider).revision ==
                     pending->second.navMeshRevision);
            if (auto* follower = scene.TryGetComponent<PathFollowerComponent>(pending->first);
                follower && follower->pathPending && navMeshStillCurrent &&
                follower->pathRequestGeneration == pending->second.generation)
            {
                follower->currentPath = std::move(path);
                follower->currentPathIndex = 0;
                follower->pathPending = false;
                follower->isMoving = !follower->currentPath.empty();
                follower->debugPlannedPath = follower->currentPath;
                follower->debugTraveledPath.clear();
                if (follower->recordDebugPath)
                {
                    if (const auto* position = scene.TryGetComponent<PositionComponent>(pending->first))
                        follower->debugTraveledPath.push_back(position->value);
                }
            }
        }
        pending = m_PendingPaths.erase(pending);
    }
    int pathRequestsStarted = 0;

    auto view =
        scene.View<PositionComponent, RotationComponent, WorldTransformComponent, PathFollowerComponent>();

    AgentGrid agentGrid;
    auto avoidanceView = scene.View<PositionComponent, PathFollowerComponent, InfoComponent>();
    if (m_SpatialHashEnabled)
    {
        agentGrid.reserve(avoidanceView.size_hint());
        for (auto agent : avoidanceView)
        {
            const auto& info = avoidanceView.get<InfoComponent>(agent);
            if (!info.isActive)
                continue;
            const auto& follower = avoidanceView.get<PathFollowerComponent>(agent);
            const bool agentFly3D = UsesThreeDimensionalMovement(follower);
            agentGrid[ToAgentCell(avoidanceView.get<PositionComponent>(agent).value, m_AgentCellSize, agentFly3D)]
                .push_back(agent);
        }
    }

    NavMeshComponent* globalNavMesh = nullptr;
    entt::entity globalNavMeshEntity = entt::null;
    auto navMeshView = scene.View<NavMeshComponent>();
    if (!navMeshView.empty())
    {
        globalNavMeshEntity = navMeshView.front();
        globalNavMesh = &navMeshView.get<NavMeshComponent>(globalNavMeshEntity);
    }

    NavigationGridComponent* globalGrid = nullptr;
    auto gridView = scene.View<NavigationGridComponent>();
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
        entt::entity selectedNavMeshEntity = globalNavMeshEntity;
        NavigationGridComponent* selectedGrid = globalGrid;

        if (follower.navigationProviderEntity != entt::null && scene.IsValid(follower.navigationProviderEntity))
        {
            if (follower.pathfindingOptions.provider == NavigationProvider::NavMesh ||
                follower.pathfindingOptions.provider == NavigationProvider::Auto)
            {
                if (auto* nav = scene.TryGetComponent<NavMeshComponent>(follower.navigationProviderEntity))
                {
                    selectedNavMesh = nav;
                    selectedNavMeshEntity = follower.navigationProviderEntity;
                }
            }
            if (follower.pathfindingOptions.provider == NavigationProvider::Grid ||
                follower.pathfindingOptions.provider == NavigationProvider::Auto)
            {
                if (auto* grid = scene.TryGetComponent<NavigationGridComponent>(follower.navigationProviderEntity);
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
            else if (m_AsyncPathfindingEnabled && pathRequestsStarted < m_MaxPathRequestsPerFrame &&
                     m_PendingPaths.find(entity) == m_PendingPaths.end() &&
                     !follower.pathfindingOptions.customCostFunc &&
                     !follower.pathfindingOptions.customGridCostFunc &&
                     !follower.pathfindingOptions.customHeuristicFunc && (selectedGrid || selectedNavMesh))
            {
                const bool useGrid = selectedGrid &&
                                     (follower.pathfindingOptions.provider == NavigationProvider::Grid ||
                                      (follower.pathfindingOptions.provider == NavigationProvider::Auto &&
                                       (!selectedNavMesh || selectedNavMesh->nodes.empty())));
                const glm::vec3 start = pos.value;
                const glm::vec3 target = follower.targetPosition;
                const PathfindingOptions options = follower.pathfindingOptions;
                std::future<std::vector<glm::vec3>> future;
                if (useGrid)
                {
                    NavigationGridComponent snapshot = *selectedGrid;
                    future = JobSystem::Instance().ExecuteAsync(
                        [start, target, options, snapshot = std::move(snapshot)]() mutable {
                            return Pathfinding::FindGridPath(start, target, snapshot, options);
                        });
                }
                else if (selectedNavMesh && !selectedNavMesh->nodes.empty())
                {
                    NavMeshComponent snapshot = *selectedNavMesh;
                    future = JobSystem::Instance().ExecuteAsync(
                        [start, target, options, snapshot = std::move(snapshot)]() mutable {
                            auto path = Pathfinding::FindPath(start, target, snapshot, options);
                            if (options.criteria == PathfindingCriteria::Smoothest)
                                path = SmoothPathCorners(path, 2);
                            return path;
                        });
                }
                if (future.valid())
                {
                    const entt::entity navProvider = useGrid ? entt::null : selectedNavMeshEntity;
                    const uint64_t navRevision = useGrid || !selectedNavMesh ? 0 : selectedNavMesh->revision;
                    m_PendingPaths.emplace(
                        entity, PendingPathRequest{follower.pathRequestGeneration, navProvider, navRevision,
                                                   std::move(future)});
                    ++pathRequestsStarted;
                }
            }
            else if (m_PendingPaths.find(entity) != m_PendingPaths.end())
            {
                // The immutable snapshot is still being processed.
            }
            else if (m_AsyncPathfindingEnabled && pathRequestsStarted >= m_MaxPathRequestsPerFrame &&
                     !follower.pathfindingOptions.customCostFunc &&
                     !follower.pathfindingOptions.customGridCostFunc &&
                     !follower.pathfindingOptions.customHeuristicFunc && (selectedGrid || selectedNavMesh))
            {
                // Keep the request pending for the next frame instead of
                // turning a burst into synchronous main-thread work.
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
            const bool fly3D = UsesThreeDimensionalMovement(follower);
            glm::vec3 target = follower.currentPath[follower.currentPathIndex];
            glm::vec3 diff = target - pos.value;
            float distance = fly3D ? glm::length(diff) : glm::length(glm::vec3(diff.x, 0.0f, diff.z));

            if (distance < (std::max)(0.001f, follower.arrivalDistance))
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
                                              fly3D, m_SpatialHashEnabled ? &agentGrid : nullptr, m_AgentCellSize);
                    if (glm::length(avoidance) > 0.001f)
                    {
                        glm::vec3 adjustedDir = moveDir + avoidance;
                        if (glm::length(adjustedDir) > 0.001f)
                            moveDir = glm::normalize(adjustedDir);
                    }
                    const float stepDistance =
                        (std::min)(moveDist, (std::max)(0.0f, follower.moveSpeed) * (std::max)(0.0f, dt));
                    glm::vec3 moveStep = moveDir * stepDistance;
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
                    scene.MarkTransformDirty(entity);
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
                            if (scene.IsValid(groundHit.entity))
                            {
                                auto* info = scene.TryGetComponent<InfoComponent>(groundHit.entity);
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

                scene.MarkTransformDirty(entity);
            }
        }
    }
}

void NavigationSystem::StopMoving(Scene& scene, entt::entity entity)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->isMoving = false;
        follower->pathPending = false;
        ++follower->pathRequestGeneration;
        m_PendingPaths.erase(entity);
        follower->currentPath.clear();
        follower->debugPlannedPath.clear();
        follower->debugTraveledPath.clear();
        follower->currentPathIndex = 0;
    }
}

bool NavigationSystem::IsMoving(Scene& scene, entt::entity entity)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    return follower ? follower->isMoving : false;
}

void NavigationSystem::SetMoveSpeed(Scene& scene, entt::entity entity, float speed)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
        follower->moveSpeed = speed;
}

float NavigationSystem::GetRemainingDistance(Scene& scene, entt::entity entity)
{
    auto* pos = scene.TryGetComponent<PositionComponent>(entity);
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);

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
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->targetPosition = position;
        ++follower->pathRequestGeneration;
        m_PendingPaths.erase(entity);
        follower->pathPending = true;
    }
}

bool NavigationSystem::HasTarget(Scene& scene, entt::entity entity)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    return follower ? (follower->pathPending || follower->isMoving) : false;
}

void NavigationSystem::Render(Scene& scene)
{
    if (!m_ShowDebug)
        return;

    auto physics_ptr = ServiceLocator::Instance().Resolve<IPhysicsWorld>();
    if (!physics_ptr)
        return;

    auto navMeshView = scene.View<NavMeshComponent>();
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

    auto pathView = scene.View<PositionComponent, PathFollowerComponent>();
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
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->pathfindingOptions.criteria = criteria;
        if (follower->pathPending)
        {
            ++follower->pathRequestGeneration;
            m_PendingPaths.erase(entity);
        }
    }
}

void NavigationSystem::SetPreferredTags(Scene& scene, entt::entity entity, const std::vector<std::string>& tags)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->pathfindingOptions.preferredTags = tags;
        if (follower->pathPending)
        {
            ++follower->pathRequestGeneration;
            m_PendingPaths.erase(entity);
        }
    }
}

void NavigationSystem::SetCustomCostFunction(Scene& scene, entt::entity entity,
                                             std::function<float(uint32_t, uint32_t, const NavMeshComponent&)> func)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->pathfindingOptions.criteria = PathfindingCriteria::Custom;
        follower->pathfindingOptions.customCostFunc = func;
        ++follower->pathRequestGeneration;
        m_PendingPaths.erase(entity);
    }
}

void NavigationSystem::SetCustomGridCostFunction(
    Scene& scene, entt::entity entity, std::function<float(uint32_t, uint32_t, const NavigationGridComponent&)> func)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->pathfindingOptions.criteria = PathfindingCriteria::Custom;
        follower->pathfindingOptions.customGridCostFunc = func;
        ++follower->pathRequestGeneration;
        m_PendingPaths.erase(entity);
    }
}

void NavigationSystem::SetNavigationProviderEntity(Scene& scene, entt::entity entity, entt::entity provider,
                                                   NavigationProvider providerType)
{
    auto* follower = scene.TryGetComponent<PathFollowerComponent>(entity);
    if (follower)
    {
        follower->navigationProviderEntity = provider;
        if (providerType != NavigationProvider::Auto)
            follower->pathfindingOptions.provider = providerType;
        if (follower->pathPending)
        {
            ++follower->pathRequestGeneration;
            m_PendingPaths.erase(entity);
        }
    }
}

bool NavigationSystem::MarkNavMeshDirty(Scene& scene, entt::entity provider, const glm::vec3& minimum,
                                        const glm::vec3& maximum)
{
    auto* navMesh = scene.TryGetComponent<NavMeshComponent>(provider);
    if (!navMesh)
        return false;
    if (m_RuntimeScene != &scene)
    {
        m_NavMeshRuntime.clear();
        m_RuntimeScene = &scene;
    }
    auto& runtime = m_NavMeshRuntime[provider];
    if (!m_NavMeshDirtyTilesEnabled || runtime.uncarvedTriangles.empty())
    {
        navMesh->needsRebuild = true;
        return true;
    }
    return QueueDirtyBounds(runtime, {glm::min(minimum, maximum), glm::max(minimum, maximum)});
}

std::vector<entt::id_type> NavigationSystem::GetReadComponents() const
{
    return {entt::type_id<PositionComponent>().hash(), entt::type_id<NavMeshComponent>().hash(),
            entt::type_id<NavigationGridComponent>().hash()};
}

std::vector<entt::id_type> NavigationSystem::GetWriteComponents() const
{
    return {entt::type_id<PositionComponent>().hash(), entt::type_id<RotationComponent>().hash(),
            entt::type_id<PathFollowerComponent>().hash(), entt::type_id<NavMeshComponent>().hash()};
}
