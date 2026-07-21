#pragma once

#include <scene/logic/octree.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <ecs/logic/entity.h>

class SceneManager;


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

    Octree* GetOctree()
    {
        return m_Octree.get();
    }

    bool IsOctreeDirty() const { return m_OctreeDirty; }
    // A caller without an entity-level change list requests a full rebuild.
    // Prefer MarkOctreeEntityDirty for ordinary transform/mesh changes.
    void SetOctreeDirty(bool dirty)
    {
        m_OctreeDirty = dirty;
        if (dirty)
            m_OctreeFullRebuildRequired = true;
        else
        {
            m_OctreeFullRebuildRequired = false;
            m_DirtyOctreeEntities.clear();
        }
    }
    void MarkOctreeEntityDirty(entt::entity entity);
    bool ConsumeOctreeChanges(std::vector<entt::entity>& output);
    void OnOctreeDirty(entt::registry&, entt::entity entity) { MarkOctreeEntityDirty(entity); }

    void MarkTransformDirty(entt::entity entity);
    void ConsumeDirtyTransforms(std::vector<entt::entity>& output);

    void InitializeManagers();
    void ShutdownManagers();

    // Entity lifecycle
    Entity CreateEntity(const std::string& name = "unnamed", const std::string& tag = "default");
    Entity CreateEntityWithTransform(const std::string& name, const glm::vec3& position,
                                     const glm::vec3& rotation = glm::vec3(0.0f),
                                     const glm::vec3& scale = glm::vec3(1.0f));
    Entity CreateEmptyEntity(const std::string& name = "Empty");

    // Hierarchy
    void SetParent(Entity child, Entity parent, bool keepWorldTransform = true);
    void SetParent(entt::entity child, entt::entity parent, bool keepWorldTransform = true)
    {
        SetParent(Entity(child, this), Entity(parent, this), keepWorldTransform);
    }
    void AddChild(Entity parent, Entity child, bool keepWorldTransform = true);
    void AddChild(entt::entity parent, entt::entity child, bool keepWorldTransform = true)
    {
        AddChild(Entity(parent, this), Entity(child, this), keepWorldTransform);
    }

    void Destroy(Entity entity);
    void Destroy(entt::entity entity)
    {
        Destroy(Entity(entity, this));
    }
    template <typename It>
    void Destroy(It first, It last)
    {
        registry.destroy(first, last);
    }
    void DestroyEntity(Entity entity, class SceneManager* manager = nullptr);
    void DestroyEntity(entt::entity entity, class SceneManager* manager = nullptr)
    {
        DestroyEntity(Entity(entity, this), manager);
    }
    void DestroyEntityWithChildren(Entity entity, class SceneManager* manager = nullptr);
    void DestroyEntityWithChildren(entt::entity entity, class SceneManager* manager = nullptr)
    {
        DestroyEntityWithChildren(Entity(entity, this), manager);
    }

    // Queries
    Entity FindByName(const std::string& name);
    Entity FindByTag(const std::string& tag);
    Entity FindByNameAndTag(const std::string& name, const std::string& tag);
    Entity FindByNameTagAndScene(const std::string& name, const std::string& tag,
                                 const std::string& sceneName);

    std::vector<Entity> FindAllByName(const std::string& name);
    std::vector<Entity> FindAllByTag(const std::string& tag);
    std::vector<Entity> FindAllBySceneName(const std::string& sceneName);

    Entity GetCameraByName(const std::string& name);
    Entity GetCameraByTag(const std::string& tag);
    std::vector<Entity> GetAllCameras();

    // Active components
    Entity GetActiveCamera();
    void SetActiveCamera(Entity entity);
    void SetActiveCamera(entt::entity entity)
    {
        SetActiveCamera(Entity(entity, this));
    }

    Entity GetActiveSkybox();
    void SetActiveSkybox(Entity entity);
    void SetActiveSkybox(entt::entity entity)
    {
        SetActiveSkybox(Entity(entity, this));
    }

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
    bool m_OctreeDirty = true;
    bool m_OctreeFullRebuildRequired = true;
    std::unordered_set<entt::entity> m_DirtyOctreeEntities;
    std::unordered_set<entt::entity> m_DirtyTransforms;

    entt::entity m_ActiveSkybox = entt::null;
    entt::entity m_ActiveCamera = entt::null;

    entt::registry registry;
};
