#pragma once

#include <navigation/logic/pathfinding.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct PathFollowerComponent
{
    glm::vec3 targetPosition = glm::vec3(0.0f);
    std::vector<glm::vec3> currentPath;
    std::vector<glm::vec3> debugPlannedPath;
    std::vector<glm::vec3> debugTraveledPath;
    uint32_t currentPathIndex = 0;

    float moveSpeed = 5.0f;
    float rotationSpeed = 10.0f;
    float maxRotationSpeed = 20.0f;
    float rotationAcceleration = 40.0f;
    float currentRotationVelocity = 0.0f;

    glm::vec3 rotationOffset = glm::vec3(0.0f);
    float arrivalDistance = 0.5f;

    bool isMoving = false;
    bool pathPending = false;
    uint64_t pathRequestGeneration = 0;
    bool recordDebugPath = true;

    PathfindingOptions pathfindingOptions;
    entt::entity navigationProviderEntity = entt::null;
    std::string navigationProviderName;

    bool lockXPitch = false;
    bool lockYYaw = false;
    bool lockZRoll = false;
    bool lockMoveX = false;
    bool lockMoveY = false;
    bool lockMoveZ = false;

    bool localAvoidanceEnabled = true;
    float separationRadius = 1.25f;
    float separationWeight = 0.85f;
    float obstacleAvoidanceDistance = 1.8f;
    float obstacleAvoidanceWeight = 1.0f;
};
