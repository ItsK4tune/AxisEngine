#include <scene/scene_loader.h>
#include <scene/scene.h>
#include <app/application.h>
#include <script/script_registry.h>
#include <utils/filesystem.h>
#include <utils/bullet_glm_helpers.h>
#include <physic/physics_loader.h>
#include <scene/component_loader.h>
#include <app/config_loader.h>

// Include handler classes
#include <scene/handlers/resource_command_handler.h>
#include <scene/handlers/entity_command_handler.h>
#include <scene/handlers/component_command_handler.h>
#include <scene/handlers/scene_validator.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

std::vector<entt::entity> SceneLoader::Load(const std::string &filePath, Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound, Application *app)
{
    std::string fullPath = FileSystem::getPath(filePath);
    std::ifstream file(fullPath);

    if (!file.is_open())
    {
        std::cerr << "[SceneLoader] Could not open scene file: " << fullPath << std::endl;
        return {};
    }

    std::vector<entt::entity> loadedEntities;
    std::map<entt::entity, std::vector<std::string>> deferredChildren;
    entt::entity currentEntity = entt::null;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        // Resource loading commands
        if (command == "LOAD_SHADER")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadShader(ss, res);
        }
        else if (command == "LOAD_MODEL")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadModel(ss, res, false);
        }
        else if (command == "LOAD_STATIC_MODEL")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadModel(ss, res, true);
        }
        else if (command == "LOAD_ANIMATION")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadAnimation(ss, res);
        }
        else if (command == "LOAD_FONT")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadFont(ss, res);
        }
        else if (command == "LOAD_SOUND")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadSound(ss, res, sound);
        }
        else if (command == "LOAD_SKYBOX")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadSkybox(ss, res);
        }
        else if (command == "LOAD_PARTICLE")
        {
            SceneHandlers::ResourceCommandHandler::HandleLoadParticle(ss, res);
        }
        // Entity commands
        else if (command == "NEW_ENTITY")
        {
            currentEntity = SceneHandlers::EntityCommandHandler::HandleNewEntity(ss, scene);
            loadedEntities.push_back(currentEntity);
        }
        else if (command == "TRANSFORM")
        {
            SceneHandlers::EntityCommandHandler::HandleTransform(ss, scene, currentEntity);
        }
        else if (command == "PARENT")
        {
            SceneHandlers::EntityCommandHandler::HandleParent(ss, scene, currentEntity);
        }
        else if (command == "CHILDREN")
        {
            SceneHandlers::EntityCommandHandler::HandleChildren(ss, currentEntity, deferredChildren);
        }
        // Component commands
        else if (command == "RENDERER")
        {
            SceneHandlers::ComponentCommandHandler::HandleRenderer(ss, scene, currentEntity, res);
        }
        else if (command == "ANIMATOR")
        {
            SceneHandlers::ComponentCommandHandler::HandleAnimator(ss, scene, currentEntity, res);
        }
        else if (command == "MATERIAL")
        {
            SceneHandlers::ComponentCommandHandler::HandleMaterial(ss, scene, currentEntity);
        }
        else if (command == "VIDEO_MAP")
        {
            SceneHandlers::ComponentCommandHandler::HandleVideoMap(ss, scene, currentEntity);
        }
        else if (command == "CAMERA")
        {
            SceneHandlers::ComponentCommandHandler::HandleCamera(ss, scene, currentEntity);
        }
        else if (command == "RIGIDBODY")
        {
            SceneHandlers::ComponentCommandHandler::HandleRigidBody(ss, scene, currentEntity, phys, file);
        }
        else if (command == "LIGHT_DIR")
        {
            SceneHandlers::ComponentCommandHandler::HandleDirectionalLight(ss, scene, currentEntity);
        }
        else if (command == "LIGHT_POINT")
        {
            SceneHandlers::ComponentCommandHandler::HandlePointLight(ss, scene, currentEntity);
        }
        else if (command == "LIGHT_SPOT")
        {
            SceneHandlers::ComponentCommandHandler::HandleSpotLight(ss, scene, currentEntity);
        }
        else if (command == "UI_TRANSFORM")
        {
            SceneHandlers::ComponentCommandHandler::HandleUITransform(ss, scene, currentEntity);
        }
        else if (command == "UI_RENDERER")
        {
            SceneHandlers::ComponentCommandHandler::HandleUIRenderer(ss, scene, currentEntity, res);
        }
        else if (command == "UI_TEXT")
        {
            SceneHandlers::ComponentCommandHandler::HandleUIText(ss, scene, currentEntity, res);
        }
        else if (command == "UI_ANIMATION")
        {
            SceneHandlers::ComponentCommandHandler::HandleUIAnimation(ss, scene, currentEntity);
        }
        else if (command == "SKYBOX_RENDERER")
        {
            SceneHandlers::ComponentCommandHandler::HandleSkyboxRenderer(ss, scene, currentEntity, res);
        }
        else if (command == "SCRIPT")
        {
            SceneHandlers::ComponentCommandHandler::HandleScript(ss, scene, currentEntity, app);
        }
        else if (command == "AUDIO_SOURCE")
        {
            SceneHandlers::ComponentCommandHandler::HandleAudioSource(ss, scene, currentEntity);
        }
        else if (command == "VIDEO_PLAYER")
        {
            SceneHandlers::ComponentCommandHandler::HandleVideoPlayer(ss, scene, currentEntity);
        }
        else if (command == "PARTICLE_EMITTER")
        {
            SceneHandlers::ComponentCommandHandler::HandleParticleEmitter(ss, scene, currentEntity, res);
        }
        else if (command == "CONFIG")
        {
            ConfigLoader::LoadConfig(ss, app);
        }
    }

    // Validate and auto-fix scene after loading
    SceneHandlers::SceneValidator::ValidateParentChildRelationships(scene, deferredChildren);
    SceneHandlers::SceneValidator::ValidateLights(scene);
    SceneHandlers::SceneValidator::ValidatePhysicsSync(scene, phys);
    
    // Note: Camera validation adds entity to scene, but we can't add to loadedEntities here
    // as it would break the return contract. Camera validation should happen elsewhere or
    // we accept that default camera isn't in loadedEntities list.
    SceneHandlers::SceneValidator::ValidateCamera(scene, app);

    return loadedEntities;
}
