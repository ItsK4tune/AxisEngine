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
            NavMeshGenerator::Generate(scene, navMesh, m_Ctx.resources, m_WalkableTags, m_CarveTags);
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
            if (globalNavMesh && !globalNavMesh->nodes.empty()) {
                LOGGER_INFO("NavigationSystem") << "Pathfinding request for entity " << (uint32_t)entity 
                            << ": Start=(" << pos.value.x << "," << pos.value.y << "," << pos.value.z << ")"
                            << " Target=(" << follower.targetPosition.x << "," << follower.targetPosition.y << "," << follower.targetPosition.z << ")"
                            << " NavMeshNodes=" << globalNavMesh->nodes.size()
                            << " Strategy=" << (int)follower.pathfindingOptions.criteria;
                            
                follower.currentPath = Pathfinding::FindPath(pos.value, follower.targetPosition, *globalNavMesh, follower.pathfindingOptions);
                follower.currentPathIndex = 0;
                follower.pathPending = false;
                follower.isMoving = !follower.currentPath.empty();
                
                // --- One-time Path Smoothing ---
                if (follower.isMoving && follower.currentPath.size() > 2) {
                    std::vector<glm::vec3> smoothedPath;
                    smoothedPath.push_back(follower.currentPath[0]);
                    
                    size_t curr = 0;
                    while (curr < follower.currentPath.size() - 1) {
                        size_t nextFound = curr + 1;
                        for (size_t test = follower.currentPath.size() - 1; test > curr + 1; --test) {
                            glm::vec3 start = follower.currentPath[curr] + glm::vec3(0, 0.5f, 0);
                            glm::vec3 end = follower.currentPath[test] + glm::vec3(0, 0.5f, 0);
                            glm::vec3 dir = end - start;
                            float dist = glm::length(dir);
                            if (dist > 0.001f) {
                                auto hit = m_Ctx.physics->Raycast(start, glm::normalize(dir), dist);
                                if (!hit.hasHit) {
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

                LOGGER_INFO("NavigationSystem") << "Pathfinding Result: " << (follower.isMoving ? "SUCCESS" : "FAILED") 
                            << " PathSize=" << follower.currentPath.size();
            } else {
                if (follower.pathPending) {
                    LOGGER_WARN("NavigationSystem") << "Path pending but NavMesh is empty or missing!";
                    follower.pathPending = false;
                    follower.isMoving = false;
                    follower.currentPath.clear();
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
                glm::vec3 moveDir = glm::vec3(diff.x, 0.0f, diff.z);
                float moveDist = glm::length(moveDir);
                if (moveDist > 0.001f) {
                    moveDir /= moveDist;
                    pos.value += moveDir * follower.moveSpeed * dt;
                } else {
                    follower.currentPathIndex++;
                    continue;
                }

                // Orientation & Ground Snap
                if (glm::length(moveDir) > 0.001f) {
                    glm::vec3 groundNormal(0, 1, 0);
                    // Use the new Raycast overload that ignores 'entity' (ourselves)
                    auto groundHit = m_Ctx.physics->Raycast(pos.value + glm::vec3(0, 100.0f, 0), glm::vec3(0, -1, 0), 200.0f, entity);
                    
                    if (groundHit.hasHit) {
                        groundNormal = groundHit.hitNormal;
                        // Added extreme value guard
                        if (!glm::any(glm::isnan(groundHit.hitPoint)) && std::abs(groundHit.hitPoint.y) < 10000.0f) {
                            pos.value.y = groundHit.hitPoint.y;
                        }
                    }

                    glm::vec3 crossDir = glm::cross(moveDir, groundNormal);
                    if (glm::length(crossDir) > 0.001f) {
                        glm::vec3 right = glm::normalize(crossDir);
                        glm::vec3 up = groundNormal;
                        glm::vec3 forward = glm::cross(up, right);

                        // Rotation Locking Logic
                        if (follower.lockXPitch || follower.lockZRoll) {
                            glm::vec3 worldUp(0, 1, 0);
                            glm::vec3 moveDirXZ(0.0f);
                            float moveLenXZ = glm::length(glm::vec3(moveDir.x, 0, moveDir.z));
                            if (moveLenXZ > 0.001f) {
                                moveDirXZ = glm::vec3(moveDir.x, 0, moveDir.z) / moveLenXZ;
                            } else {
                                // Default forward if moving only vertically
                                moveDirXZ = glm::vec3(0, 0, 1); 
                            }
                            
                            if (follower.lockXPitch && follower.lockZRoll) {
                                // Only Yaw
                                up = worldUp;
                                glm::vec3 crossXZ = glm::cross(moveDirXZ, worldUp);
                                if (glm::length(crossXZ) > 0.001f) {
                                    right = glm::normalize(crossXZ);
                                } else {
                                    right = glm::vec3(1, 0, 0); // Default if moveDirXZ is somehow parallel to worldUp
                                }
                                forward = moveDirXZ;
                            } else if (follower.lockXPitch) {
                                // Keep forward flat on XZ, but allow roll
                                forward = moveDirXZ;
                                up = glm::normalize(glm::cross(right, forward));
                            } else if (follower.lockZRoll) {
                                // Keep right flat on XZ, but allow pitch
                                glm::vec3 crossXZ = glm::cross(forward, worldUp);
                                if (glm::length(crossXZ) > 0.001f) {
                                    right = glm::normalize(crossXZ);
                                } else {
                                    right = glm::vec3(1, 0, 0);
                                }
                                up = glm::normalize(glm::cross(right, forward));
                            }
                        }

                        if (glm::length(forward) > 0.001f) {
                            glm::mat3 rotMat;
                            rotMat[0] = right;
                            rotMat[1] = up;
                            rotMat[2] = -forward;
                            glm::quat targetRot = glm::quat_cast(rotMat);
                            
                            if (follower.lockYYaw) {
                                // This would be unusual, but let's handle it: keep original yaw
                                // (Harder to decompose, but we can slerp and then reset yaw if needed)
                                // For now, we'll just allow it unless explicitly requested otherwise.
                            }

                            if (glm::length(follower.rotationOffset) > 0.001f) {
                                glm::quat offsetQuat = glm::quat(glm::radians(follower.rotationOffset));
                                targetRot = targetRot * offsetQuat;
                            }
                            
                            float dot = 0.0f;
                            float angleDiff = 0.0f;
                            
                            if (glm::length(rot.value) > 0.001f && glm::length(targetRot) > 0.001f) {
                                dot = glm::dot(glm::normalize(rot.value), glm::normalize(targetRot));
                                angleDiff = glm::acos(glm::min(glm::abs(dot), 1.0f)) * 2.0f;
                            }
                            
                            if (angleDiff > 0.001f && !glm::isnan(angleDiff)) {
                                follower.currentRotationVelocity += follower.rotationAcceleration * dt;
                                if (follower.currentRotationVelocity > follower.maxRotationSpeed) {
                                    follower.currentRotationVelocity = follower.maxRotationSpeed;
                                }
                                
                                float step = (follower.currentRotationVelocity * dt) / angleDiff;
                                if (step > 1.0f) step = 1.0f;
                                if (!glm::isnan(step) && step > 0.0f) {
                                    rot.value = glm::slerp(rot.value, targetRot, step);
                                }
                            } else {
                                follower.currentRotationVelocity = 0.0f;
                                if (!std::isnan(targetRot.x) && !std::isnan(targetRot.y) && !std::isnan(targetRot.z) && !std::isnan(targetRot.w) && glm::length(targetRot) > 0.001f) {
                                    rot.value = targetRot;
                                }
                            }
                        }
                    }
                }

                world.isDirty = true;
            }
        }
    }
}

void NavigationSystem::StopMoving(Scene& scene, entt::entity entity)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower) {
        follower->isMoving = false;
        follower->pathPending = false;
        follower->currentPath.clear();
        follower->currentPathIndex = 0;
    }
}

bool NavigationSystem::IsMoving(Scene& scene, entt::entity entity)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    return follower ? follower->isMoving : false;
}

void NavigationSystem::SetMoveSpeed(Scene& scene, entt::entity entity, float speed)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower) follower->moveSpeed = speed;
}

float NavigationSystem::GetRemainingDistance(Scene& scene, entt::entity entity)
{
    auto* pos = scene.registry.try_get<PositionComponent>(entity);
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    
    if (!pos || !follower || !follower->isMoving || follower->currentPath.empty()) 
        return 0.0f;

    float dist = 0.0f;
    glm::vec3 lastPoint = pos->value;

    for (size_t i = follower->currentPathIndex; i < follower->currentPath.size(); ++i) {
        dist += glm::distance(lastPoint, follower->currentPath[i]);
        lastPoint = follower->currentPath[i];
    }

    return dist;
}

void NavigationSystem::MoveTo(Scene& scene, entt::entity entity, const glm::vec3& position)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower) {
        follower->targetPosition = position;
        follower->pathPending = true;
    }
}

bool NavigationSystem::HasTarget(Scene& scene, entt::entity entity)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    return follower ? (follower->pathPending || follower->isMoving) : false;
}

void NavigationSystem::Render(Scene& scene)
{
    if (!m_ShowDebug) return;

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

void NavigationSystem::AddWalkableTag(const std::string& tag)
{
    if (std::find(m_WalkableTags.begin(), m_WalkableTags.end(), tag) == m_WalkableTags.end()) {
        m_WalkableTags.push_back(tag);
    }
}

void NavigationSystem::ClearWalkableTags()
{
    m_WalkableTags.clear();
    m_WalkableTags.push_back("Walkable");
}

void NavigationSystem::AddCarveTag(const std::string& tag)
{
    if (std::find(m_CarveTags.begin(), m_CarveTags.end(), tag) == m_CarveTags.end()) {
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
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower) follower->pathfindingOptions.criteria = criteria;
}

void NavigationSystem::SetPreferredTags(Scene& scene, entt::entity entity, const std::vector<std::string>& tags)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower) follower->pathfindingOptions.preferredTags = tags;
}

void NavigationSystem::SetCustomCostFunction(Scene& scene, entt::entity entity, std::function<float(uint32_t, uint32_t, const NavMeshComponent&)> func)
{
    auto* follower = scene.registry.try_get<PathFollowerComponent>(entity);
    if (follower) {
        follower->pathfindingOptions.criteria = PathfindingCriteria::Custom;
        follower->pathfindingOptions.customCostFunc = func;
    }
}

std::vector<entt::id_type> NavigationSystem::GetReadComponents() const
{
    return {
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<NavMeshComponent>().hash()
    };
}

std::vector<entt::id_type> NavigationSystem::GetWriteComponents() const
{
    return {
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<PathFollowerComponent>().hash()
    };
}
