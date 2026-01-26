#pragma once

#include <entt/entt.hpp>
#include <ecs/component.h>
#include <scene/light_manager.h>
#include <scene/camera_manager.h>
#include <scene/entity_factory.h>
#include <memory>

class SceneManager;

struct Scene
{
    entt::registry registry;

    entt::entity createEntity();
    void destroyEntity(entt::entity entity, SceneManager *manager = nullptr);

    entt::entity GetActiveCamera();
    void SetActiveCamera(entt::entity entity);

    entt::entity GetActiveSkybox() const;
    void SetActiveSkybox(entt::entity entity);

    LightManager &GetLightManager() { return *lightManager; }
    CameraManager &GetCameraManager() { return *cameraManager; }
    EntityFactory &GetEntityFactory() { return *entityFactory; }

    void InitializeManagers();
    void ShutdownManagers();

private:
    std::unique_ptr<LightManager> lightManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<EntityFactory> entityFactory;
    
    entt::entity m_ActiveSkybox = entt::null;
    entt::entity m_ActiveCamera = entt::null;
};
