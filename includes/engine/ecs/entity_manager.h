#pragma once

#include <scene/scene.h>
#include <vector>
#include <string>

class EntityManager
{
public:
    static entt::entity CreateEntity(Scene& scene, const std::string &name = "unnamed", const std::string &tag = "default");
    static entt::entity CreateEntityWithTransform(Scene& scene, const std::string &name, const glm::vec3 &position, const glm::vec3 &rotation = glm::vec3(0.0f), const glm::vec3 &scale = glm::vec3(1.0f));

    static entt::entity CreateEmptyEntity(Scene& scene, const std::string &name = "Empty");
    static entt::entity CreateCube(Scene& scene, const std::string &name, const glm::vec3 &position);
    static entt::entity CreateSphere(Scene& scene, const std::string &name, const glm::vec3 &position);
    static entt::entity CreatePlane(Scene& scene, const std::string &name, const glm::vec3 &position);

    static void SetParent(Scene& scene, entt::entity child, entt::entity parent, bool keepWorldTransform = true);
    static void AddChild(Scene& scene, entt::entity parent, entt::entity child, bool keepWorldTransform = true);

    static void Destroy(Scene& scene, entt::entity entity);
    static void DestroyEntity(Scene& scene, entt::entity entity, class SceneManager *manager = nullptr);
    static void DestroyEntityWithChildren(Scene& scene, entt::entity entity, class SceneManager *manager = nullptr);

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

    static entt::entity GetActiveCamera(Scene& scene);
    static void SetActiveCamera(Scene& scene, entt::entity entity);

    static entt::entity GetActiveSkybox(Scene& scene);
    static void SetActiveSkybox(Scene& scene, entt::entity entity);

    // --- Template Utilities ---
    template<typename T, typename... Args>
    static T& AddComponent(Scene& scene, entt::entity entity, Args&&... args)
    {
        return scene.registry.emplace<T>(entity, std::forward<Args>(args)...);
    }

    template<typename T>
    static T& GetComponent(Scene& scene, entt::entity entity)
    {
        return scene.registry.get<T>(entity);
    }

    template<typename T>
    static T* TryGetComponent(Scene& scene, entt::entity entity)
    {
        return scene.registry.try_get<T>(entity);
    }

    template<typename T>
    static bool HasComponent(Scene& scene, entt::entity entity)
    {
        return scene.registry.all_of<T>(entity);
    }

    template<typename T>
    static void RemoveComponent(Scene& scene, entt::entity entity)
    {
        scene.registry.remove<T>(entity);
    }

    static bool IsValid(Scene& scene, entt::entity entity)
    {
        return scene.registry.valid(entity);
    }
};
