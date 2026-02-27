#pragma once

#include <entt/entt.hpp>
#include <ecs/component.h>
#include <scene/light_manager.h>
#include <scene/camera_manager.h>
#include <scene/entity_factory.h>
#include <scene/octree.h>
#include <memory>

class SceneManager;

struct Scene
{
    Scene();
    ~Scene();

    entt::registry registry;

    entt::entity createEntity();
    void destroyEntity(entt::entity entity, SceneManager *manager = nullptr);

    entt::entity GetActiveCamera();
    void SetActiveCamera(entt::entity entity);

    entt::entity GetActiveSkybox() const;
    void SetActiveSkybox(entt::entity entity);

    void OnScriptComponentDestroyed(entt::registry &reg, entt::entity entity);

    LightManager &GetLightManager() { return *lightManager; }
    CameraManager &GetCameraManager() { return *cameraManager; }
    EntityFactory &GetEntityFactory() { return *entityFactory; }
    Octree* GetOctree() { return m_Octree.get(); }

    void InitializeManagers();
    void ShutdownManagers();

private:
    std::unique_ptr<LightManager> lightManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<EntityFactory> entityFactory;
    std::unique_ptr<Octree> m_Octree;

    entt::entity m_ActiveSkybox = entt::null;
    entt::entity m_ActiveCamera = entt::null;
};
