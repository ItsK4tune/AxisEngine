#include <navigation/logic/navigation_system.h>
#include <navigation/logic/navmesh_generator.h>
#include <navigation/logic/pathfinding.h>
#include <ecs/unit/core_components.h>
#include <physics/interface/i_physics_world.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <core/logic/logger.h>

void NavigationSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled) return;

    UpdateNavMesh(scene);
    UpdatePathFollowing(scene, dt);
}

void NavigationSystem::UpdateNavMesh(Scene& scene)
{
    auto view = scene.registry.view<NavMeshComponent>();
    for (auto entity : view) {
        auto& navMesh = view.get<NavMeshComponent>(entity);
        if (navMesh.needsRebuild) {
            LOGGER_INFO("NavigationSystem") << "NavMesh rebuild triggered for entity " << (uint32_t)entity;
            NavMeshGenerator::Generate(scene, navMesh);
            LOGGER_INFO("NavigationSystem") << "NavMesh state: Nodes=" << navMesh.nodes.size() << ", Tris=" << navMesh.triangles.size();
        }
    }
}

void NavigationSystem::UpdatePathFollowing(Scene& scene, float dt)
{
    auto view = scene.registry.view<PositionComponent, RotationComponent, WorldTransformComponent, PathFollowerComponent>();
    
    // Find NavMesh (global singleton-like component for now)
    NavMeshComponent* globalNavMesh = nullptr;
    auto navMeshView = scene.registry.view<NavMeshComponent>();
    if (!navMeshView.empty()) {
        globalNavMesh = &navMeshView.get<NavMeshComponent>(navMeshView.front());
    }

    for (auto entity : view) {
        auto& pos = view.get<PositionComponent>(entity);
        auto& rot = view.get<RotationComponent>(entity);
        auto& world = view.get<WorldTransformComponent>(entity);
        auto& follower = view.get<PathFollowerComponent>(entity);

        if (follower.pathPending) {
            if (globalNavMesh) {
                LOGGER_INFO("NavigationSystem") << "Pathfinding request: Start=" << pos.value.x << "," << pos.value.z << " End=" << follower.targetPosition.x << "," << follower.targetPosition.z;
                follower.currentPath = Pathfinding::FindPath(pos.value, follower.targetPosition, *globalNavMesh);
                follower.currentPathIndex = 0;
                follower.pathPending = false;
                follower.isMoving = !follower.currentPath.empty();
                LOGGER_INFO("NavigationSystem") << "Pathfound: " << (follower.isMoving ? "YES" : "NO") << " Nodes=" << follower.currentPath.size();
            } else {
                LOGGER_WARN("NavigationSystem") << "Path pending but no global NavMesh found!";
                follower.pathPending = false;
            }
        }

        // --- Path Smoothing (Farthest Visible Node) ---
        if (follower.isMoving && follower.currentPathIndex + 1 < follower.currentPath.size()) {
            for (size_t nextIdx = follower.currentPath.size() - 1; nextIdx > follower.currentPathIndex; --nextIdx) {
                glm::vec3 startPos = pos.value + glm::vec3(0, 0.5f, 0);
                glm::vec3 targetPos = follower.currentPath[nextIdx] + glm::vec3(0, 0.5f, 0);
                glm::vec3 dir = targetPos - startPos;
                float dist = glm::length(dir);
                if (dist > 0.001f) {
                    auto hit = m_Ctx.physics->Raycast(startPos, glm::normalize(dir), dist);
                    if (!hit.hasHit) {
                        follower.currentPathIndex = (uint32_t)nextIdx;
                        break;
                    }
                }
            }
        }

        if (follower.isMoving && !follower.currentPath.empty()) {
            glm::vec3 target = follower.currentPath[follower.currentPathIndex];
            glm::vec3 diff = target - pos.value;
            float distance = glm::length(glm::vec3(diff.x, 0.0f, diff.z));

            if (distance < follower.arrivalDistance) {
                follower.currentPathIndex++;
                if (follower.currentPathIndex >= follower.currentPath.size()) {
                    follower.isMoving = false;
                    follower.currentPath.clear();
                }
            } else {
                glm::vec3 moveDir = glm::normalize(glm::vec3(diff.x, 0.0f, diff.z));
                pos.value += moveDir * follower.moveSpeed * dt;

                // Orientation with Surface Alignment & Accel-based Rotation
                if (glm::length(moveDir) > 0.001f) {
                    glm::vec3 groundNormal(0, 1, 0);
                    auto groundHit = m_Ctx.physics->Raycast(pos.value + glm::vec3(0, 1.0f, 0), glm::vec3(0, -1, 0), 2.0f);
                    if (groundHit.hasHit) {
                        groundNormal = groundHit.hitNormal;
                    }

                    glm::vec3 right = glm::normalize(glm::cross(moveDir, groundNormal));
                    glm::vec3 actualForward = glm::normalize(glm::cross(groundNormal, right));
                    
                    glm::mat3 rotMat;
                    rotMat[0] = right;
                    rotMat[1] = groundNormal;
                    rotMat[2] = -actualForward;
                    glm::quat targetRot = glm::quat_cast(rotMat);
                    
                    if (glm::length(follower.rotationOffset) > 0.001f) {
                        glm::quat offsetQuat = glm::quat(glm::radians(follower.rotationOffset));
                        targetRot = targetRot * offsetQuat;
                    }
                    
                    // Acceleration-based rotation
                    float dot = glm::dot(glm::normalize(rot.value), glm::normalize(targetRot));
                    float angleDiff = glm::acos(glm::min(glm::abs(dot), 1.0f)) * 2.0f;
                    if (angleDiff > 0.001f) {
                        follower.currentRotationVelocity += follower.rotationAcceleration * dt;
                        if (follower.currentRotationVelocity > follower.maxRotationSpeed) {
                            follower.currentRotationVelocity = follower.maxRotationSpeed;
                        }
                        
                        float step = (follower.currentRotationVelocity * dt) / angleDiff;
                        if (step > 1.0f) step = 1.0f;
                        
                        rot.value = glm::slerp(rot.value, targetRot, step);
                    } else {
                        follower.currentRotationVelocity = 0.0f;
                        rot.value = targetRot;
                    }
                }

                world.isDirty = true;
            }
        }
    }
}

void NavigationSystem::Render(Scene& scene)
{
    // Debug visualization of NavMesh
    auto navMeshView = scene.registry.view<NavMeshComponent>();
    for (auto entity : navMeshView) {
        auto& navMesh = navMeshView.get<NavMeshComponent>(entity);
        
        for (const auto& tri : navMesh.triangles) {
            glm::vec3 v0 = navMesh.vertices[tri.indices[0]];
            glm::vec3 v1 = navMesh.vertices[tri.indices[1]];
            glm::vec3 v2 = navMesh.vertices[tri.indices[2]];
            
            // Draw edges in blue
            m_Ctx.physics->DrawLine(v0, v1, glm::vec3(0, 0, 1));
            m_Ctx.physics->DrawLine(v1, v2, glm::vec3(0, 0, 1));
            m_Ctx.physics->DrawLine(v2, v0, glm::vec3(0, 0, 1));
        }
    }

    // Debug visualization of paths
    auto pathView = scene.registry.view<PositionComponent, PathFollowerComponent>();
    for (auto entity : pathView) {
        auto& follower = pathView.get<PathFollowerComponent>(entity);
        if (follower.isMoving && !follower.currentPath.empty()) {
            for (size_t i = 1; i < follower.currentPath.size(); ++i) {
                glm::vec3 p0 = follower.currentPath[i-1];
                glm::vec3 p1 = follower.currentPath[i];
                // Draw path in green
                m_Ctx.physics->DrawLine(p0, p1, glm::vec3(0, 1, 0));
            }
        }
    }
}
