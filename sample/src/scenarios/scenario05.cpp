#include "sample_scenario_common.h"

void SampleState::LoadScene5()
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
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(ground))
        renderer->color = glm::vec4(0.1f, 0.85f, 0.2f, 1.0f);

    auto navSurface = EntityBuilder(scene, res, "scenario")
        .WithName("NavigationGridSurface")
        .WithTag("walkable")
        .WithTransform(glm::vec3(-25.0f, 0.0f, -25.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .Build();
    auto& terrain = scene.registry.emplace<TerrainComponent>(navSurface);
    terrain.terrainSize = glm::vec3(50.0f, 0.0f, 50.0f);
    terrain.maxHeight = 0.0f;
    terrain.isWalkable = true;
    terrain.generatePhysics = false;

    const auto makeRoad = [&](const char* name, const glm::vec3& pos, const glm::vec3& scale) {
        auto road = EntityBuilder(scene, res, "scenario")
            .WithName(name)
            .WithTag("road")
            .WithTransform(pos, glm::vec3(0.0f), scale)
            .WithPBRMesh("cubeModel", "deferred_unlit", 0.0f, 0.55f, 1.0f)
            .Build();
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(road))
            renderer->color = glm::vec4(0.08f, 0.09f, 0.1f, 1.0f);
    };
    makeRoad("RoadNorthSouth", glm::vec3(-20.0f, 0.08f, 0.0f), glm::vec3(5.0f, 0.1f, 45.0f));
    makeRoad("RoadEastWest", glm::vec3(0.0f, 0.09f, -20.0f), glm::vec3(45.0f, 0.1f, 5.0f));

    auto groundPhys = EntityBuilder(scene, res, "scenario")
        .WithName("GroundPhys")
        .WithTransform(glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f))
        .Build();
    auto& groundShape = EntityManager::AddComponent<RigidShapeComponent>(scene, groundPhys);
    groundShape.type = ShapeType::Box;
    groundShape.size = glm::vec3(50.0f, 1.0f, 50.0f);
    auto& groundRB = EntityManager::AddComponent<RigidBodyComponent>(scene, groundPhys);
    groundRB.mass = 0.0f;
    groundRB.isStatic = true;

    // Dynamically generate obstacles based on count and size parameters
    for (int i = 0; i < m_S5ObstacleCount; ++i)
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
        glm::vec3 obstacleScale(m_S5ObstacleSize, obstacleHeight, m_S5ObstacleSize);

        auto obstacle = EntityBuilder(scene, res, "scenario")
            .WithName("Obstacle_" + std::to_string(i))
            .WithTag("obstacle")
            .WithTransform(glm::vec3(ox, obstacleHeight * 0.5f, oz), glm::vec3(0.0f), obstacleScale)
            .WithPBRMesh("cubeModel", "deferred_lit", 0.3f, 0.3f, 1.0f)
            .Build();
        if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(obstacle))
            renderer->color = glm::vec4(0.9f, 0.08f, 0.05f, 1.0f);

        auto obstacleCollider = EntityBuilder(scene, res, "scenario")
            .WithName("ObstacleCollider_" + std::to_string(i))
            .WithTransform(glm::vec3(ox, obstacleHeight * 0.5f, oz), glm::vec3(0.0f), glm::vec3(1.0f))
            .Build();

        auto& shape = EntityManager::AddComponent<RigidShapeComponent>(scene, obstacleCollider);
        shape.type = ShapeType::Box;
        shape.size = obstacleScale;
        shape.friction = 0.8f;
        auto& rb = EntityManager::AddComponent<RigidBodyComponent>(scene, obstacleCollider);
        rb.mass = 0.0f;
        rb.isStatic = true;
    }

    auto navMesh = scene.registry.create();
    scene.registry.emplace<InfoComponent>(navMesh).sceneName = "scenario";
    auto& navComp = scene.registry.emplace<NavMeshComponent>(navMesh);
    navComp.needsRebuild = true;
    navComp.isDynamic = true;

    m_NavFollower = EntityBuilder(scene, res, "scenario")
        .WithName("Follower")
        .WithTransform(glm::vec3(-20.0f, 2.5f, 20.0f), glm::vec3(0.0f), glm::vec3(1.5f))
        .WithPBRMesh("capsuleModel", "deferred_lit", 0.1f, 0.5f, 1.0f)
        .WithPathFollower(m_S5FollowerSpeed, 15.0f, 30.0f, 60.0f)
        .Build();
    if (auto* renderer = scene.registry.try_get<MeshRendererComponent>(m_NavFollower))
        renderer->color = glm::vec4(0.1f, 0.9f, 0.25f, 1.0f);

    auto& pf = scene.registry.get<PathFollowerComponent>(m_NavFollower);
    pf.lockXPitch = m_S5LockXPitch;
    pf.lockYYaw = m_S5LockYYaw;
    pf.lockZRoll = m_S5LockZRoll;
    pf.lockMoveX = m_S5LockMoveX;
    pf.lockMoveY = m_S5LockMoveY;
    pf.lockMoveZ = m_S5LockMoveZ;
    ConfigureScenario5PathOptions(pf, m_S5PathfindingCriteria);
    pf.recordDebugPath = true;

    if (m_S5PathfindingCriteria == 0)
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    else if (m_S5PathfindingCriteria == 1)
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(-8.0f, 0.5f, 10.0f),
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(10.0f, 0.5f, -8.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    else if (m_S5PathfindingCriteria == 2)
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(-20.0f, 0.5f, -20.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    else if (m_S5PathfindingCriteria == 3)
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(-12.0f, 0.5f, 12.0f),
            glm::vec3(0.0f, 0.5f, 0.0f),
            glm::vec3(12.0f, 0.5f, -12.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    else if (m_S5PathfindingCriteria == 4)
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(-18.0f, 1.5f, 12.0f),
            glm::vec3(-6.0f, 2.5f, 2.0f),
            glm::vec3(8.0f, 3.5f, -8.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    else
    {
        m_NavWaypoints = {
            glm::vec3(-20.0f, 0.5f, 20.0f),
            glm::vec3(-6.0f, 4.0f, 4.0f),
            glm::vec3(8.0f, 2.0f, -8.0f),
            glm::vec3(20.0f, 0.5f, -20.0f)
        };
    }
    m_CurrentWaypointIndex = 1;
    m_S5LastPathfindingCriteria = m_S5PathfindingCriteria;
    m_S5RepathRequested = false;

    navSystem.SetShowDebug(m_ShowDebugLines);
    auto& transformSys = GetSystem<TransformSystem>();
    transformSys.m_IsLinearTransformsDirty = true;
    transformSys.Update(scene, 0.0f);
    auto& physicsSystem = GetSystem<PhysicsSystem>();
    physicsSystem.Update(scene, 0.0f);
    navSystem.Update(scene, 0.0f);
    auto navMeshView = scene.registry.view<NavMeshComponent>();
    for (auto entity : navMeshView)
    {
        auto& navMesh = navMeshView.get<NavMeshComponent>(entity);
        for (auto& tri : navMesh.triangles)
        {
            bool onRoad = (std::abs(tri.center.x + 20.0f) <= 2.9f && tri.center.z >= -22.5f && tri.center.z <= 22.5f) ||
                          (std::abs(tri.center.z + 20.0f) <= 2.9f && tri.center.x >= -22.5f && tri.center.x <= 22.5f);
            tri.tag = onRoad ? "road" : "walkable";
        }
        for (auto& node : navMesh.nodes)
        {
            bool onRoad = (std::abs(node.position.x + 20.0f) <= 2.9f && node.position.z >= -22.5f &&
                           node.position.z <= 22.5f) ||
                          (std::abs(node.position.z + 20.0f) <= 2.9f && node.position.x >= -22.5f &&
                           node.position.x <= 22.5f);
            node.tag = onRoad ? "road" : "walkable";
        }
    }
    navSystem.MoveTo(scene, m_NavFollower, m_NavWaypoints[m_CurrentWaypointIndex]);
}
