#pragma once

#include <scene/logic/octree.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class SceneManager;

#define GLM_ENABLE_EXPERIMENTAL

struct Scene
{
    Scene();
    ~Scene();

    // EnTT registry access
    entt::registry& GetRegistry() { return registry; }
    const entt::registry& GetRegistry() const { return registry; }

    size_t GetEntityCount() const { return registry.storage<entt::entity>()->size(); }

    template <typename... T>
    auto View()
    {
        return registry.view<T...>();
    }

    template <typename... T>
    auto View() const
    {
        return registry.view<T...>();
    }

    friend class EntityBuilder;
    friend class SceneSerializer;
    friend class BinarySceneSerializer;
    friend class SceneManager;
    friend class ComponentLoader;
    friend class FragmentLoader;
    friend class SceneValidator;

    Octree* GetOctree()
    {
        return m_Octree.get();
    }

    void InitializeManagers();
    void ShutdownManagers();

    // Entity lifecycle
    entt::entity CreateEntity(const std::string& name = "unnamed", const std::string& tag = "default");
    entt::entity CreateEntityWithTransform(const std::string& name, const glm::vec3& position,
                                           const glm::vec3& rotation = glm::vec3(0.0f),
                                           const glm::vec3& scale = glm::vec3(1.0f));
    entt::entity CreateEmptyEntity(const std::string& name = "Empty");
    entt::entity CreateCube(const std::string& name, const glm::vec3& position);
    entt::entity CreateSphere(const std::string& name, const glm::vec3& position);
    entt::entity CreatePlane(const std::string& name, const glm::vec3& position);

    // Hierarchy
    void SetParent(entt::entity child, entt::entity parent, bool keepWorldTransform = true);
    void AddChild(entt::entity parent, entt::entity child, bool keepWorldTransform = true);

    void Destroy(entt::entity entity);
    template <typename It>
    void Destroy(It first, It last)
    {
        registry.destroy(first, last);
    }
    void DestroyEntity(entt::entity entity, class SceneManager* manager = nullptr);
    void DestroyEntityWithChildren(entt::entity entity, class SceneManager* manager = nullptr);

    // Queries
    entt::entity FindByName(const std::string& name);
    entt::entity FindByTag(const std::string& tag);
    entt::entity FindByNameAndTag(const std::string& name, const std::string& tag);
    entt::entity FindByNameTagAndScene(const std::string& name, const std::string& tag,
                                       const std::string& sceneName);

    std::vector<entt::entity> FindAllByName(const std::string& name);
    std::vector<entt::entity> FindAllByTag(const std::string& tag);
    std::vector<entt::entity> FindAllBySceneName(const std::string& sceneName);

    entt::entity GetCameraByName(const std::string& name);
    entt::entity GetCameraByTag(const std::string& tag);
    std::vector<entt::entity> GetAllCameras();

    // Active components
    entt::entity GetActiveCamera();
    void SetActiveCamera(entt::entity entity);

    entt::entity GetActiveSkybox();
    void SetActiveSkybox(entt::entity entity);

    // Component templates
    template <typename T, typename... Args>
    T& AddComponent(entt::entity entity, Args&&... args)
    {
        return registry.emplace<T>(entity, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T& AddOrReplaceComponent(entt::entity entity, Args&&... args)
    {
        return registry.emplace_or_replace<T>(entity, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T& GetOrAddComponent(entt::entity entity, Args&&... args)
    {
        return registry.get_or_emplace<T>(entity, std::forward<Args>(args)...);
    }

    template <typename... T>
    decltype(auto) GetComponent(entt::entity entity)
    {
        return registry.get<T...>(entity);
    }

    template <typename... T>
    decltype(auto) GetComponent(entt::entity entity) const
    {
        return registry.get<T...>(entity);
    }

    template <typename... T>
    decltype(auto) TryGetComponent(entt::entity entity)
    {
        return registry.try_get<T...>(entity);
    }

    template <typename... T>
    decltype(auto) TryGetComponent(entt::entity entity) const
    {
        return registry.try_get<T...>(entity);
    }

    template <typename... T>
    bool HasAllComponents(entt::entity entity) const
    {
        return registry.all_of<T...>(entity);
    }

    template <typename... T>
    bool HasAnyComponent(entt::entity entity) const
    {
        return registry.any_of<T...>(entity);
    }

    template <typename T>
    void RemoveComponent(entt::entity entity)
    {
        registry.remove<T>(entity);
    }

    bool IsValid(entt::entity entity) const
    {
        return registry.valid(entity);
    }

private:
    std::unique_ptr<Octree> m_Octree;

    entt::entity m_ActiveSkybox = entt::null;
    entt::entity m_ActiveCamera = entt::null;

    entt::registry registry;
};
