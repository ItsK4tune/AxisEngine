#pragma once

#include <scene/scene.h>
#include <vector>
#include <string>

class EntityManager
{
public:
    static void Destroy(Scene& scene, entt::entity entity);

    static entt::entity FindByName(Scene& scene, const std::string& name);
    static entt::entity FindByTag(Scene& scene, const std::string& tag);
    static entt::entity FindByNameAndTag(Scene& scene, const std::string& name, const std::string& tag);
    static entt::entity FindByNameTagAndScene(Scene& scene, const std::string& name, const std::string& tag, const std::string& sceneName);

    static std::vector<entt::entity> FindAllByName(Scene& scene, const std::string& name);
    static std::vector<entt::entity> FindAllByTag(Scene& scene, const std::string& tag);
    static std::vector<entt::entity> FindAllBySceneName(Scene& scene, const std::string& sceneName);

    // --- Component Specific Queries ---
    static entt::entity GetCameraByName(Scene& scene, const std::string& name);
    static entt::entity GetCameraByTag(Scene& scene, const std::string& tag);
    static std::vector<entt::entity> GetAllCameras(Scene& scene);
};
