#pragma once

#include "sample_state.h"

#include <core/logic/filesystem.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

constexpr const char* kScenario25BaseSceneName = "scenario_base";
constexpr const char* kScenario25DynamicSceneName = "scenario";

inline std::string SamplePath(const char* relativePath)
{
    std::filesystem::path path(relativePath);
    if (std::filesystem::exists(path))
        return std::filesystem::absolute(path).lexically_normal().generic_string();

    std::filesystem::path parentPath = std::filesystem::path("..") / path;
    if (std::filesystem::exists(parentPath))
        return std::filesystem::absolute(parentPath).lexically_normal().generic_string();

    std::filesystem::path rootedPath(FileSystem::getPath(relativePath));
    if (std::filesystem::exists(rootedPath))
        return std::filesystem::absolute(rootedPath).lexically_normal().generic_string();

    return path.generic_string();
}

inline float Scenario23NavHeight(float x, float z)
{
    float ridge = 4.0f * std::exp(-std::abs(x) * 0.075f);
    float roll = 1.2f * std::sin(z * 0.18f) + 0.8f * std::sin((x + z) * 0.11f);
    return 0.5f + (std::max)(0.0f, ridge + roll);
}

inline float Scenario23WaypointY(int criteria, float x, float z)
{
    return (criteria == 0 || criteria == 1 || criteria == 4) ? Scenario23NavHeight(x, z) : 0.5f;
}

inline void ConfigureScenario23FollowerOptions(PathFollowerComponent& follower, int criteria)
{
    follower.pathfindingOptions.criteria = static_cast<PathfindingCriteria>(criteria);
    follower.pathfindingOptions.preferredTags =
        criteria == 2 ? std::vector<std::string>{"road"} : std::vector<std::string>{"walkable", "road"};
    follower.pathfindingOptions.tagWeightBonus = criteria == 2 ? 80.0f : 1.0f;
    follower.pathfindingOptions.altitudePenaltyWeight = criteria == 1 ? 14.0f : 5.0f;
}

inline glm::quat RotationFromNegativeY(const glm::vec3& direction)
{
    glm::vec3 dir = glm::normalize(direction);
    if (glm::length(dir) < 0.001f)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), dir);
}

inline void EnsureScenario28AuxBindings(InputManager& input)
{
    if (!input.HasAction("PlayerJump"))
        input.BindAction("PlayerJump", InputType::Key, (int)Key::Space);
    if (!input.HasAction("PlayerAction"))
        input.BindAction("PlayerAction", InputType::MouseButton, (int)Mouse::Left);
}
