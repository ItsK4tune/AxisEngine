#pragma once

#include "sample_state.h"

#include <audio/interface/i_sound.h>
#include <audio/logic/audio_service.h>
#include <core/logic/data_node_serializer.h>
#include <ecs/logic/post_process_system.h>
#include <ecs/unit/light_probe_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/network_components.h>
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

constexpr const char* kScenario25BaseSceneName = "scenario_base";
constexpr const char* kScenario25DynamicSceneName = "scenario";

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

inline void ConfigureScenario23PathOptions(PathFollowerComponent& follower, int criteria)
{
    follower.pathfindingOptions.criteria = static_cast<PathfindingCriteria>(criteria);
    follower.pathfindingOptions.preferredTags =
        criteria == 2 ? std::vector<std::string>{"road"} : std::vector<std::string>{"walkable", "road"};
    follower.pathfindingOptions.tagWeightBonus = criteria == 2 ? 80.0f : 1.0f;
    follower.pathfindingOptions.altitudePenaltyWeight = criteria == 1 ? 14.0f : 5.0f;
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

inline void ResetRigidShape(RigidShapeComponent& shape)
{
    shape.children.clear();
    shape.offset = glm::vec3(0.0f);
    shape.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    shape.size = glm::vec3(1.0f);
    shape.radius = 1.0f;
    shape.height = 2.0f;
}

inline void ConfigureBoxCollider(RigidShapeComponent& shape, const glm::vec3& localHalfExtents = glm::vec3(1.0f))
{
    ResetRigidShape(shape);
    shape.type = ShapeType::Box;
    shape.size = localHalfExtents;
}

inline void ConfigurePrimitiveCollider(RigidShapeComponent& shape, ShapeType type)
{
    ResetRigidShape(shape);
    shape.type = type;

    if (type == ShapeType::Box)
    {
        shape.size = glm::vec3(1.0f);
    }
    else if (type == ShapeType::Sphere)
    {
        shape.radius = 1.0f;
    }
    else if (type == ShapeType::Capsule)
    {
        shape.radius = 1.0f;
        shape.height = 2.0f;
    }
    else if (type == ShapeType::Cylinder)
    {
        shape.radius = 1.0f;
        shape.height = 2.0f;
    }
}

inline glm::quat RotationFromNegativeY(const glm::vec3& direction)
{
    glm::vec3 dir = glm::normalize(direction);
    if (glm::length(dir) < 0.001f)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    return glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), dir);
}

struct Scenario21ShapeSpec
{
    const char* mesh;
    ShapeType shape;
    glm::vec3 visualScale;
    glm::vec3 boxSize;
    float radius;
    float height;
};

inline Scenario21ShapeSpec GetScenario21ShapeSpec(int shapeIndex, bool payload)
{
    switch (shapeIndex)
    {
        case 1:
            return {"sphereModel",   ShapeType::Sphere,       payload ? glm::vec3(1.65f) : glm::vec3(0.85f),
                    glm::vec3(1.0f), payload ? 0.65f : 0.45f, payload ? 1.0f : 0.8f};
        case 2:
            return {"capsuleModel",
                    ShapeType::Capsule,
                    payload ? glm::vec3(1.35f, 1.9f, 1.35f) : glm::vec3(0.65f, 1.15f, 0.65f),
                    glm::vec3(1.0f),
                    payload ? 0.55f : 0.42f,
                    payload ? 0.9f : 0.55f};
        default:
            return {"cubeModel",
                    ShapeType::Box,
                    payload ? glm::vec3(1.6f) : glm::vec3(0.75f, 0.95f, 0.75f),
                    payload ? glm::vec3(0.58f) : glm::vec3(0.46f),
                    payload ? 1.0f : 0.5f,
                    payload ? 1.0f : 0.8f};
    }
}

inline void ApplyShapeSpec(RigidShapeComponent& shape, const Scenario21ShapeSpec& spec)
{
    ConfigurePrimitiveCollider(shape, spec.shape);
    if (spec.shape == ShapeType::Box)
        shape.size = spec.boxSize;
    else if (spec.shape == ShapeType::Sphere)
        shape.radius = spec.radius;
    else if (spec.shape == ShapeType::Capsule || spec.shape == ShapeType::Cylinder)
    {
        shape.radius = spec.radius;
        shape.height = spec.height;
    }
}

inline void EnsureScenario28AuxBindings(InputManager& input)
{
    if (!input.HasAction("PlayerJump"))
        input.BindAction("PlayerJump", InputType::Key, (int)Key::Space);
    if (!input.HasAction("PlayerAction"))
        input.BindAction("PlayerAction", InputType::MouseButton, (int)Mouse::Left);
}
