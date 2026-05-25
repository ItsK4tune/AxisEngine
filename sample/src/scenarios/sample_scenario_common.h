#pragma once

#include "sample_state.h"

#include <audio/interface/i_sound.h>
#include <audio/logic/audio_service.h>
#include <core/logic/data_node_serializer.h>
#include <ecs/logic/post_process_system.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/post_process_component.h>
#include <ecs/unit/reflection_components.h>
#include <network/network_system.h>
#include <physics/logic/collision_matrix.h>
#include <platform/logic/input_serializer.h>
#include <scene/logic/scene_serializer.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <physics/type/shape_type.h>
#include <glm/gtx/quaternion.hpp>

constexpr const char* kScenario12BaseSceneName = "scenario_base";
constexpr const char* kScenario12DynamicSceneName = "scenario";

inline std::string SamplePath(const char* relativePath)
{
    std::filesystem::path path(relativePath);
    if (std::filesystem::exists(path))
        return path.generic_string();

    std::filesystem::path parentPath = std::filesystem::path("..") / path;
    if (std::filesystem::exists(parentPath))
        return parentPath.generic_string();

    return path.generic_string();
}

inline void ConfigureScenario5PathOptions(PathFollowerComponent& follower, int criteria)
{
    follower.pathfindingOptions.criteria = static_cast<PathfindingCriteria>(criteria);
    follower.pathfindingOptions.preferredTags = {"road"};
    follower.pathfindingOptions.tagWeightBonus = 30.0f;
    follower.pathfindingOptions.altitudePenaltyWeight = 10.0f;
}

inline glm::quat RotationFromNegativeY(const glm::vec3& direction)
{
    glm::vec3 dir = glm::normalize(direction);
    if (glm::length(dir) < 0.001f)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), dir);
}

struct Scenario10ShapeSpec
{
    const char* mesh;
    ShapeType shape;
    glm::vec3 visualScale;
    glm::vec3 boxSize;
    float radius;
    float height;
};

inline Scenario10ShapeSpec GetScenario10ShapeSpec(int shapeIndex, bool payload)
{
    switch (shapeIndex)
    {
    case 1:
        return {"sphereModel", ShapeType::Sphere, payload ? glm::vec3(2.0f) : glm::vec3(0.95f), glm::vec3(1.0f),
                payload ? 1.0f : 0.48f, payload ? 1.0f : 0.8f};
    case 2:
        return {"capsuleModel", ShapeType::Capsule, payload ? glm::vec3(1.5f, 2.2f, 1.5f) :
                                                              glm::vec3(0.7f, 1.25f, 0.7f),
                glm::vec3(0.8f, 1.0f, 0.8f), payload ? 0.75f : 0.35f, payload ? 1.4f : 0.9f};
    default:
        return {"cubeModel", ShapeType::Box, payload ? glm::vec3(2.0f) : glm::vec3(0.8f, 1.0f, 0.8f),
                payload ? glm::vec3(2.0f) : glm::vec3(0.8f, 1.0f, 0.8f), payload ? 1.0f : 0.5f,
                payload ? 1.0f : 0.8f};
    }
}

inline void ApplyShapeSpec(RigidShapeComponent& shape, const Scenario10ShapeSpec& spec)
{
    shape.type = spec.shape;
    shape.size = spec.boxSize;
    shape.radius = spec.radius;
    shape.height = spec.height;
}

inline void EnsureScenario13AuxBindings(InputManager& input)
{
    if (!input.HasAction("PlayerJump"))
        input.BindAction("PlayerJump", InputType::Key, (int)Key::Space);
    if (!input.HasAction("PlayerAction"))
        input.BindAction("PlayerAction", InputType::MouseButton, (int)Mouse::Left);
}
