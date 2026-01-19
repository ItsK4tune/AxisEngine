#pragma once

#include <entt/entt.hpp>
#include <ecs/component.h>
#include <vector>
#include <glm/glm.hpp>

class Scene;
class Application;

class CameraManager
{
public:
    CameraManager(Scene& scene);
    ~CameraManager();

    // Primary camera management
    entt::entity GetPrimaryCamera() const;
    void SetPrimaryCamera(entt::entity entity);
    
    // Query methods
    std::vector<entt::entity> GetAllCameras() const;
    
    // Validation
    void EnsurePrimaryCamera(Application* app);
    
    // Camera creation helpers
    entt::entity CreateCamera(const glm::vec3& position, float fov = 45.0f, float yaw = -90.0f, float pitch = 0.0f, bool isPrimary = false);
    entt::entity CreateDefaultSpectatorCamera(Application* app);

    // Camera properties
    glm::mat4 GetViewMatrix(entt::entity camera) const;
    glm::mat4 GetProjectionMatrix(entt::entity camera, float aspectRatio) const;
    glm::vec3 GetCameraPosition(entt::entity camera) const;
    glm::vec3 GetCameraDirection(entt::entity camera) const;

private:
    Scene& m_Scene;
};
