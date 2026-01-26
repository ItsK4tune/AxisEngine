#include <scene/handlers/component_command_handler.h>
#include <scene/scene.h>
#include <scene/component_loader.h>
#include <physic/physics_loader.h>
#include <resource/resource_manager.h>
#include <app/application.h>
#include <ecs/component.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace SceneHandlers
{
    void ComponentCommandHandler::HandleRenderer(std::stringstream &ss, Scene &scene, entt::entity entity, ResourceManager &res)
    {
        ComponentLoader::LoadRenderer(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleAnimator(std::stringstream &ss, Scene &scene, entt::entity entity, ResourceManager &res)
    {
        ComponentLoader::LoadAnimator(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleMaterial(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        ComponentLoader::LoadMaterial(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleVideoMap(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        float sx = 1.0f, sy = 1.0f, ox = 0.0f, oy = 0.0f;
        ss >> sx >> sy >> ox >> oy;

        auto &mat = scene.registry.get_or_emplace<MaterialComponent>(entity);
        mat.uvScale = glm::vec2(sx, sy);
        mat.uvOffset = glm::vec2(ox, oy);
    }

    void ComponentCommandHandler::HandleDirectionalLight(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        int activeFlag = 1;
        int castShadowFlag = 0;
        float r, g, b, i;

        ss >> activeFlag >> castShadowFlag >> r >> g >> b >> i;

        auto &l = scene.registry.emplace<DirectionalLightComponent>(entity);
        l.active = (activeFlag != 0);
        l.isCastShadow = (castShadowFlag != 0);
        l.color = glm::vec3(r, g, b);
        l.intensity = i;

        float ambientStr = 0.2f;
        float diffuseStr = 0.8f;

        if (ss >> ambientStr)
        {
            if (ss >> diffuseStr)
            {
            }
        }

        l.ambient = l.color * ambientStr;
        l.diffuse = l.color * diffuseStr;
        l.specular = glm::vec3(0.5f);
    }

    void ComponentCommandHandler::HandlePointLight(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        int activeFlag = 1;
        int castShadowFlag = 0;
        float r, g, b, i, rad;
        
        ss >> activeFlag >> castShadowFlag >> r >> g >> b >> i >> rad;

        auto &l = scene.registry.emplace<PointLightComponent>(entity);
        l.active = (activeFlag != 0);
        l.isCastShadow = (castShadowFlag != 0);
        l.color = glm::vec3(r, g, b);
        l.intensity = i;
        l.radius = rad;

        float c = 1.0f, lin = 0.09f, quad = 0.032f;
        if (ss >> c >> lin >> quad)
        {
            l.constant = c;
            l.linear = lin;
            l.quadratic = quad;
        }

        float ambStr = 0.1f;
        float diffStr = 1.0f;
        if (ss >> ambStr)
        {
            if (ss >> diffStr)
            {
            }
        }
        l.ambient = l.color * ambStr;
        l.diffuse = l.color * diffStr;
        l.specular = glm::vec3(1.0f);
    }

    void ComponentCommandHandler::HandleSpotLight(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        int activeFlag = 1;
        int castShadowFlag = 0;
        float r, g, b, i, cut, outer;
        
        ss >> activeFlag >> castShadowFlag >> r >> g >> b >> i >> cut >> outer;

        auto &l = scene.registry.emplace<SpotLightComponent>(entity);
        l.active = (activeFlag != 0);
        l.isCastShadow = (castShadowFlag != 0);
        l.color = glm::vec3(r, g, b);
        l.intensity = i;
        l.cutOff = glm::cos(glm::radians(cut));
        l.outerCutOff = glm::cos(glm::radians(outer));

        float c = 1.0f, lin = 0.09f, quad = 0.032f;
        if (ss >> c >> lin >> quad)
        {
            l.constant = c;
            l.linear = lin;
            l.quadratic = quad;
        }

        float ambStr = 0.1f;
        float diffStr = 1.0f;
        if (ss >> ambStr)
        {
            if (ss >> diffStr)
            {
            }
        }
        l.ambient = l.color * ambStr;
        l.diffuse = l.color * diffStr;
        l.specular = glm::vec3(1.0f);
    }

    // Camera
    void ComponentCommandHandler::HandleCamera(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        ComponentLoader::LoadCamera(scene, entity, ss);
        // Force last loaded camera to be active
        scene.SetActiveCamera(entity);
    }

    // Physics
    void ComponentCommandHandler::HandleRigidBody(std::stringstream &ss, Scene &scene, entt::entity entity,
                                                  PhysicsWorld &phys, std::ifstream &file)
    {
        PhysicsLoader::LoadRigidBody(scene, entity, ss, phys, file);
    }

    // UI
    void ComponentCommandHandler::HandleUITransform(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        ComponentLoader::LoadUITransform(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleUIRenderer(std::stringstream &ss, Scene &scene, entt::entity entity, ResourceManager &res)
    {
        ComponentLoader::LoadUIRenderer(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleUIText(std::stringstream &ss, Scene &scene, entt::entity entity, ResourceManager &res)
    {
        ComponentLoader::LoadUIText(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleUIAnimation(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        ComponentLoader::LoadUIAnimation(scene, entity, ss);
    }

    // Others
    void ComponentCommandHandler::HandleSkyboxRenderer(std::stringstream &ss, Scene &scene, entt::entity entity, ResourceManager &res)
    {
        ComponentLoader::LoadSkyboxRenderer(scene, entity, ss, res);
        // Force last loaded skybox to be active
        scene.SetActiveSkybox(entity);
    }

    void ComponentCommandHandler::HandleScript(std::stringstream &ss, Scene &scene, entt::entity entity, Application *app)
    {
        ComponentLoader::LoadScript(scene, entity, ss, app);
    }

    void ComponentCommandHandler::HandleAudioSource(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        ComponentLoader::LoadAudioSource(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleVideoPlayer(std::stringstream &ss, Scene &scene, entt::entity entity)
    {
        ComponentLoader::LoadVideoPlayer(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleParticleEmitter(std::stringstream &ss, Scene &scene, entt::entity entity, ResourceManager &res)
    {
        ComponentLoader::LoadParticleEmitter(scene, entity, ss, res);
    }
}
