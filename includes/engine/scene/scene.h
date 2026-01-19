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

    // Legacy methods
    entt::entity createEntity();
    void destroyEntity(entt::entity entity, SceneManager* manager = nullptr);
    
    entt::entity GetActiveCamera();
    
    // Manager accessors
    LightManager& GetLightManager() { return *lightManager; }
    CameraManager& GetCameraManager() { return *cameraManager; }
    EntityFactory& GetEntityFactory() { return *entityFactory; }

    // Initialization
    void InitializeManagers();
    void ShutdownManagers();

private:
    std::unique_ptr<LightManager> lightManager;
    std::unique_ptr<CameraManager> cameraManager;
    std::unique_ptr<EntityFactory> entityFactory;
};
