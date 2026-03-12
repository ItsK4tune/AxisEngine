#pragma once

#include <glm/glm.hpp>
#include <vector>

struct PathFollowerComponent
{
    glm::vec3 targetPosition = glm::vec3(0.0f);
    std::vector<glm::vec3> currentPath;
    uint32_t currentPathIndex = 0;
    
    float moveSpeed = 5.0f;
    float rotationSpeed = 10.0f; // Current/Target rotation speed
    float maxRotationSpeed = 20.0f;
    float rotationAcceleration = 40.0f;
    float currentRotationVelocity = 0.0f; 
    
    glm::vec3 rotationOffset = glm::vec3(0.0f); // Euler offsets in degrees
    float arrivalDistance = 0.5f;
    
    bool isMoving = false;
    bool pathPending = false;

    // Rotation locking
    bool lockXPitch = false;
    bool lockYYaw = false;
    bool lockZRoll = false;
};
