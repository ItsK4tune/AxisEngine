#include "test_framework.h"

#include <navigation/logic/pathfinding.h>

namespace
{
NavMeshNode Node(glm::vec3 position, std::vector<uint32_t> neighbors, std::string tag = "walkable")
{
    NavMeshNode node;
    node.position = position;
    node.neighbors = std::move(neighbors);
    node.triangleIndex = 0;
    node.tag = std::move(tag);
    return node;
}
}  // namespace

AXIS_TEST_CASE("Pathfinding returns straight line without navmesh")
{
    NavMeshComponent navMesh;
    PathfindingOptions options;
    options.criteria = PathfindingCriteria::StraightLine;

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {5.0f, 0.0f, 0.0f}, navMesh, options);

    AXIS_CHECK(path.size() == 2);
    AXIS_CHECK_NEAR(path[0].x, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(path[1].x, 5.0f, 0.0001f);
}

AXIS_TEST_CASE("Pathfinding returns empty path for empty navmesh")
{
    NavMeshComponent navMesh;

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {5.0f, 0.0f, 0.0f}, navMesh);

    AXIS_CHECK(path.empty());
}

AXIS_TEST_CASE("Pathfinding returns empty path for disconnected graph")
{
    NavMeshComponent navMesh;
    navMesh.nodes = {
        Node({0.0f, 0.0f, 0.0f}, {}),
        Node({10.0f, 0.0f, 0.0f}, {}),
    };

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, navMesh);

    AXIS_CHECK(path.empty());
}

AXIS_TEST_CASE("Pathfinding shortest criteria chooses lower distance route")
{
    NavMeshComponent navMesh;
    navMesh.nodes = {
        Node({0.0f, 0.0f, 0.0f}, {1, 3}),
        Node({1.0f, 0.0f, 0.0f}, {0, 2}),
        Node({2.0f, 0.0f, 0.0f}, {1, 3}),
        Node({0.0f, 0.0f, 5.0f}, {0, 2}),
    };

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, navMesh);

    AXIS_CHECK(path.size() >= 4);
    AXIS_CHECK_NEAR(path[1].x, 1.0f, 0.0001f);
    AXIS_CHECK_NEAR(path[1].z, 0.0f, 0.0001f);
}

AXIS_TEST_CASE("Pathfinding stay-on-road criteria favors preferred tags")
{
    NavMeshComponent navMesh;
    navMesh.nodes = {
        Node({0.0f, 0.0f, 0.0f}, {1, 2}),
        Node({1.0f, 0.0f, 0.0f}, {0, 4}, "mud"),
        Node({1.0f, 0.0f, 0.1f}, {0, 3}, "road"),
        Node({1.5f, 0.0f, 0.1f}, {2, 4}, "road"),
        Node({2.0f, 0.0f, 0.0f}, {1, 3}),
    };

    PathfindingOptions options;
    options.criteria = PathfindingCriteria::StayOnRoad;
    options.preferredTags = {"road"};
    options.tagWeightBonus = 80.0f;

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, navMesh, options);

    AXIS_CHECK(path.size() >= 5);
    AXIS_CHECK_NEAR(path[1].z, 0.1f, 0.0001f);
    AXIS_CHECK_NEAR(path[2].z, 0.1f, 0.0001f);
}

AXIS_TEST_CASE("Pathfinding smoothest criteria avoids large height jumps")
{
    NavMeshComponent navMesh;
    navMesh.nodes = {
        Node({0.0f, 0.0f, 0.0f}, {1, 3}),
        Node({1.0f, 8.0f, 0.0f}, {0, 2}),
        Node({2.0f, 0.0f, 0.0f}, {1, 4}),
        Node({0.0f, 0.0f, 2.0f}, {0, 4}),
        Node({2.0f, 0.0f, 2.0f}, {2, 3}),
    };

    PathfindingOptions options;
    options.criteria = PathfindingCriteria::Smoothest;
    options.altitudePenaltyWeight = 10.0f;

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, navMesh, options);

    AXIS_CHECK(path.size() >= 4);
    AXIS_CHECK_NEAR(path[1].y, 0.0f, 0.0001f);
    AXIS_CHECK_NEAR(path[1].z, 2.0f, 0.0001f);
}

AXIS_TEST_CASE("Pathfinding custom cost function can block a neighbor")
{
    NavMeshComponent navMesh;
    navMesh.nodes = {
        Node({0.0f, 0.0f, 0.0f}, {1, 2}),
        Node({1.0f, 0.0f, 0.0f}, {0, 3}),
        Node({0.0f, 0.0f, 1.0f}, {0, 3}),
        Node({2.0f, 0.0f, 0.0f}, {1, 2}),
    };

    PathfindingOptions options;
    options.criteria = PathfindingCriteria::Custom;
    options.customCostFunc = [](uint32_t, uint32_t neighbor, const NavMeshComponent&) {
        return neighbor == 1 ? 100.0f : 1.0f;
    };

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, navMesh, options);

    AXIS_CHECK(path.size() >= 4);
    AXIS_CHECK_NEAR(path[1].z, 1.0f, 0.0001f);
}

AXIS_TEST_CASE("Pathfinding high-ground criteria rewards higher nodes")
{
    NavMeshComponent navMesh;
    navMesh.nodes = {
        Node({0.0f, 0.0f, 0.0f}, {1, 2}),
        Node({1.0f, 0.0f, 4.0f}, {0, 3}, "low"),
        Node({1.0f, 2.0f, 0.0f}, {0, 3}, "high"),
        Node({2.0f, 0.0f, 0.0f}, {1, 2}),
    };

    PathfindingOptions options;
    options.criteria = PathfindingCriteria::HighGround;

    auto path = Pathfinding::FindPath({0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, navMesh, options);

    AXIS_CHECK(path.size() >= 4);
    AXIS_CHECK_NEAR(path[1].y, 2.0f, 0.0001f);
}
