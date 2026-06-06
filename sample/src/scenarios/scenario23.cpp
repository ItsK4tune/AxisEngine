#include "sample_scenario_common.h"

void SampleState::LoadScene23()
{
    auto& scene = GetScene();
    auto& res = Get<ResourceManager>();
    auto& navSystem = GetSystem<NavigationSystem>();
    navSystem.ClearWalkableTags();
    navSystem.AddWalkableTag("walkable");
    navSystem.AddWalkableTag("road");
    navSystem.ClearCarveTags();
    navSystem.AddCarveTag("obstacle");

    EntityBuilder(scene, res, "scenario")
        .WithName("DirLight")
        .WithTransform(glm::vec3(0.0f, 40.0f, 0.0f), glm::vec3(-45.0f, -45.0f, 0.0f))
        .WithDirectionalLight(glm::normalize(glm::vec3(-0.7f, -1.0f, -0.7f)), glm::vec3(1.0f), 1.2f)
        .Build();

    auto ground = EntityBuilder(scene, res, "scenario")
                      .WithName("Ground")
                      .WithTag("walkable")
                      .WithTransform(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(50.0f, 1.0f, 50.0f))
                      .WithPBRMesh("planeModel", "deferred_lit", 0.0f, 0.9f, 1.0f)
                      .Build();
    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(ground))
        renderer->color = glm::vec4(0.1f, 0.85f, 0.2f, 1.0f);

    EntityBuilder(scene, res, "scenario")
        .WithName("NavigationGridSurface")
        .WithTag("walkable")
        .WithActive(false)
        .WithTransform(glm::vec3(-25.0f, 0.0f, -25.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .WithTerrain(glm::vec3(50.0f, 0.0f, 50.0f), 0.0f, true, false)
        .Build();

    const auto makeRoad = [&](const char* name, const glm::vec3& pos, const glm::vec3& scale) {
        auto road = EntityBuilder(scene, res, "scenario")
                        .WithName(name)
                        .WithTag("road")
                        .WithTransform(pos, glm::vec3(0.0f), scale)
                        .WithPBRMesh("cubeModel", "deferred_unlit", 0.0f, 0.55f, 1.0f)
                        .Build();
        if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(road))
            renderer->color = glm::vec4(0.08f, 0.09f, 0.1f, 1.0f);
    };
    makeRoad("RoadNorthSouth", glm::vec3(-20.0f, 0.08f, 0.0f), glm::vec3(5.0f, 0.1f, 45.0f));
    makeRoad("RoadEastWest", glm::vec3(0.0f, 0.09f, -20.0f), glm::vec3(45.0f, 0.1f, 5.0f));

    EntityBuilder(scene, res, "scenario")
        .WithName("GroundPhys")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .WithRigidShape(ShapeType::Box, glm::vec3(50.0f, 1.0f, 50.0f))
        .WithRigidBody(0.0f, true)
        .Build();

    // Dynamically generate obstacles based on count and size parameters
    for (int i = 0; i < m_S23ObstacleCount; ++i)
    {
        float ox = static_cast<float>(rand() % 36 - 18);
        float oz = static_cast<float>(rand() % 36 - 18);
        if (glm::length(glm::vec2(ox + 20.0f, oz - 20.0f)) < 6.0f ||
            glm::length(glm::vec2(ox - 20.0f, oz + 20.0f)) < 6.0f)
        {
            ox += 10.0f;
            oz -= 10.0f;
        }

        float obstacleHeight = 2.0f + static_cast<float>(rand() % 50) / 10.0f;
        glm::vec3 obstacleScale(m_S23ObstacleSize, obstacleHeight, m_S23ObstacleSize);

        auto obstacle = EntityBuilder(scene, res, "scenario")
                            .WithName("Obstacle_" + std::to_string(i))
                            .WithTag("obstacle")
                            .WithTransform(glm::vec3(ox, obstacleHeight * 0.5f, oz), glm::vec3(0.0f), obstacleScale)
                            .WithPBRMesh("cubeModel", "deferred_lit", 0.3f, 0.3f, 1.0f)
                            .Build();
        if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(obstacle))
            renderer->color = glm::vec4(0.9f, 0.08f, 0.05f, 1.0f);

        EntityBuilder(scene, res, "scenario")
            .WithName("ObstacleCollider_" + std::to_string(i))
            .WithTransform(glm::vec3(ox, obstacleHeight * 0.5f, oz), glm::vec3(0.0f), glm::vec3(1.0f))
            .WithRigidShape(ShapeType::Box, obstacleScale, 1.0f, 2.0f, 0.8f)
            .WithRigidBody(0.0f, true)
            .Build();
    }

    auto navMesh = EntityBuilder(scene, res, "scenario")
                       .WithName("NavMesh")
                       .WithNavMesh(true)
                       .Build();

    m_NavFollower = EntityBuilder(scene, res, "scenario")
                        .WithName("Follower")
                        .WithTransform(glm::vec3(-20.0f, 2.5f, 20.0f), glm::vec3(0.0f), glm::vec3(1.5f))
                        .WithPBRMesh("capsuleModel", "deferred_lit", 0.1f, 0.5f, 1.0f)
                        .WithPathFollower(m_S23FollowerSpeed, 15.0f, 30.0f, 60.0f)
                        .Build();
    if (auto* renderer = scene.TryGetComponent<MeshRendererComponent>(m_NavFollower))
        renderer->color = glm::vec4(0.1f, 0.9f, 0.25f, 1.0f);

    auto& pf = scene.GetComponent<PathFollowerComponent>(m_NavFollower);
    pf.lockXPitch = m_S23LockXPitch;
    pf.lockYYaw = m_S23LockYYaw;
    pf.lockZRoll = m_S23LockZRoll;
    pf.lockMoveX = m_S23LockMoveX;
    pf.lockMoveY = m_S23LockMoveY;
    pf.lockMoveZ = m_S23LockMoveZ;
    ConfigureScenario23PathOptions(pf, m_S23PathfindingCriteria);
    pf.recordDebugPath = true;

    const auto waypoint = [this](float x, float z) {
        return glm::vec3(x, Scenario23WaypointY(m_S23PathfindingCriteria, x, z), z);
    };

    if (m_S23PathfindingCriteria == 0)
    {
        m_NavWaypoints = {waypoint(-22.0f, 22.0f), waypoint(22.0f, -22.0f)};
    }
    else if (m_S23PathfindingCriteria == 1)
    {
        m_NavWaypoints = {waypoint(-22.0f, 22.0f), waypoint(-12.0f, 14.0f), waypoint(-2.0f, 3.0f),
                          waypoint(11.0f, -10.0f), waypoint(22.0f, -22.0f)};
    }
    else if (m_S23PathfindingCriteria == 2)
    {
        m_NavWaypoints = {waypoint(-22.0f, 22.0f), waypoint(-20.0f, -20.0f), waypoint(22.0f, -20.0f)};
    }
    else if (m_S23PathfindingCriteria == 3)
    {
        m_NavWaypoints = {waypoint(-22.0f, 22.0f), waypoint(22.0f, -22.0f)};
    }
    else if (m_S23PathfindingCriteria == 4)
    {
        m_NavWaypoints = {waypoint(-22.0f, 22.0f), waypoint(-5.0f, 16.0f), waypoint(0.0f, 2.0f), waypoint(5.0f, -16.0f),
                          waypoint(22.0f, -22.0f)};
    }
    else
    {
        m_NavWaypoints = {glm::vec3(-20.0f, 0.5f, 20.0f), glm::vec3(-6.0f, 4.0f, 4.0f), glm::vec3(8.0f, 2.0f, -8.0f),
                          glm::vec3(20.0f, 0.5f, -20.0f)};
    }
    m_CurrentWaypointIndex = 1;
    if (!m_NavWaypoints.empty())
    {
        if (auto* pos = scene.TryGetComponent<PositionComponent>(m_NavFollower))
            pos->value = m_NavWaypoints.front();
        if (auto* world = scene.TryGetComponent<WorldTransformComponent>(m_NavFollower))
            world->isDirty = true;
    }
    m_S23LastPathfindingCriteria = m_S23PathfindingCriteria;
    m_S23RepathRequested = false;

    navSystem.SetShowDebug(m_ShowDebugLines);
    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(scene, 0.0f);
    auto& physicsSystem = GetSystem<PhysicsSystem>();
    physicsSystem.Update(scene, 0.0f);
    navSystem.Update(scene, 0.0f);
    auto navMeshView = scene.View<NavMeshComponent>();
    for (auto entity : navMeshView)
    {
        auto& navMesh = navMeshView.get<NavMeshComponent>(entity);
        for (auto& tri : navMesh.triangles)
        {
            bool onRoad = (std::abs(tri.center.x + 20.0f) <= 2.9f && tri.center.z >= -22.5f && tri.center.z <= 22.5f) ||
                          (std::abs(tri.center.z + 20.0f) <= 2.9f && tri.center.x >= -22.5f && tri.center.x <= 22.5f);
            tri.tag = onRoad ? "road" : "walkable";
            tri.center.y = onRoad ? 0.5f : Scenario23NavHeight(tri.center.x, tri.center.z);
        }
        for (auto& node : navMesh.nodes)
        {
            bool onRoad =
                (std::abs(node.position.x + 20.0f) <= 2.9f && node.position.z >= -22.5f && node.position.z <= 22.5f) ||
                (std::abs(node.position.z + 20.0f) <= 2.9f && node.position.x >= -22.5f && node.position.x <= 22.5f);
            node.tag = onRoad ? "road" : "walkable";
            node.position.y = onRoad ? 0.5f : Scenario23NavHeight(node.position.x, node.position.z);
        }
    }
    navSystem.MoveTo(scene, m_NavFollower, m_NavWaypoints[m_CurrentWaypointIndex]);
}
