#include <scene/scene_loader.h>
#include <utils/logger.h>
#include <scene/scene.h>
#include <app/application.h>
#include <script/script_registry.h>
#include <utils/filesystem.h>
#include <utils/bullet_glm_helpers.h>
#include <physic/physics_loader.h>
#include <scene/component_loader.h>
#include <app/config_loader.h>

#include <scene/handlers/resource_command_handler.h>
#include <scene/handlers/entity_command_handler.h>
#include <scene/handlers/component_command_handler.h>
#include <scene/handlers/scene_validator.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <unordered_map>
#include <functional>

std::vector<entt::entity> SceneLoader::Load(const std::string &filePath, Scene &scene, ResourceManager &res, PhysicsWorld &phys, SoundManager &sound, Application *app)
{
    std::string fullPath = FileSystem::getPath(filePath);
    std::ifstream file(fullPath);

    if (!file.is_open())
    {
        LOGGER_ERROR("SceneLoader") << "Could not open scene file: " << fullPath;
        return {};
    }

    std::vector<entt::entity> loadedEntities;
    std::map<entt::entity, std::vector<std::string>> deferredChildren;
    entt::entity currentEntity = entt::null;

    using HandlerFunc = std::function<void(std::stringstream&)>;
    std::unordered_map<std::string, HandlerFunc> dispatchMap;

    // Resource loading commands
    dispatchMap["LOAD_SHADER"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadShader(ss, res); };
    dispatchMap["LOAD_MODEL"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadModel(ss, res, false); };
    dispatchMap["LOAD_STATIC_MODEL"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadModel(ss, res, true); };
    dispatchMap["LOAD_ANIMATION"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadAnimation(ss, res); };
    dispatchMap["LOAD_FONT"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadFont(ss, res); };
    dispatchMap["LOAD_SOUND"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadSound(ss, res, sound); };
    dispatchMap["LOAD_SKYBOX"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadSkybox(ss, res); };
    dispatchMap["LOAD_PARTICLE"] = [&](std::stringstream& ss) { SceneHandlers::ResourceCommandHandler::HandleLoadParticle(ss, res); };

    // Entity commands
    dispatchMap["NEW_ENTITY"] = [&](std::stringstream& ss) { 
        currentEntity = SceneHandlers::EntityCommandHandler::HandleNewEntity(ss, scene);
        loadedEntities.push_back(currentEntity);
    };
    dispatchMap["TRANSFORM"] = [&](std::stringstream& ss) { SceneHandlers::EntityCommandHandler::HandleTransform(ss, scene, currentEntity); };
    dispatchMap["PARENT"] = [&](std::stringstream& ss) { SceneHandlers::EntityCommandHandler::HandleParent(ss, scene, currentEntity); };
    dispatchMap["CHILDREN"] = [&](std::stringstream& ss) { SceneHandlers::EntityCommandHandler::HandleChildren(ss, currentEntity, deferredChildren); };

    // Component commands
    dispatchMap["RENDERER"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleRenderer(ss, scene, currentEntity, res); };
    dispatchMap["ANIMATOR"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleAnimator(ss, scene, currentEntity, res); };
    dispatchMap["MATERIAL"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleMaterial(ss, scene, currentEntity); };
    dispatchMap["VIDEO_MAP"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleVideoMap(ss, scene, currentEntity); };
    dispatchMap["CAMERA"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleCamera(ss, scene, currentEntity); };
    dispatchMap["RIGIDBODY"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleRigidBody(ss, scene, currentEntity, phys, file); };
    dispatchMap["LIGHT_DIR"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleDirectionalLight(ss, scene, currentEntity); };
    dispatchMap["LIGHT_POINT"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandlePointLight(ss, scene, currentEntity); };
    dispatchMap["LIGHT_SPOT"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleSpotLight(ss, scene, currentEntity); };
    dispatchMap["UI_TRANSFORM"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleUITransform(ss, scene, currentEntity); };
    dispatchMap["UI_RENDERER"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleUIRenderer(ss, scene, currentEntity, res); };
    dispatchMap["UI_TEXT"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleUIText(ss, scene, currentEntity, res); };
    dispatchMap["UI_ANIMATION"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleUIAnimation(ss, scene, currentEntity); };
    dispatchMap["SKYBOX_RENDERER"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleSkyboxRenderer(ss, scene, currentEntity, res); };
    dispatchMap["SCRIPT"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleScript(ss, scene, currentEntity, app); };
    dispatchMap["AUDIO_SOURCE"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleAudioSource(ss, scene, currentEntity); };
    dispatchMap["VIDEO_PLAYER"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleVideoPlayer(ss, scene, currentEntity); };
    dispatchMap["PARTICLE_EMITTER"] = [&](std::stringstream& ss) { SceneHandlers::ComponentCommandHandler::HandleParticleEmitter(ss, scene, currentEntity, res); };
    dispatchMap["CONFIG"] = [&](std::stringstream& ss) { ConfigLoader::LoadConfig(ss, app); };

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        auto it = dispatchMap.find(command);
        if (it != dispatchMap.end())
        {
            it->second(ss);
        }
    }

    SceneHandlers::SceneValidator::ValidateParentChildRelationships(scene, deferredChildren);
    SceneHandlers::SceneValidator::ValidateLights(scene);
    SceneHandlers::SceneValidator::ValidatePhysicsSync(scene, phys);

    SceneHandlers::SceneValidator::ValidateCamera(scene, app);

    LOGGER_INFO("SceneLoader") << "Finished parsing scene file: " << fullPath << ". loaded " << loadedEntities.size() << " entities.";
    return loadedEntities;
}
