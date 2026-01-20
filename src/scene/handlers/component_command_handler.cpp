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
    // Rendering
    void ComponentCommandHandler::HandleRenderer(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res)
    {
        ComponentLoader::LoadRenderer(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleAnimator(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res)
    {
        ComponentLoader::LoadAnimator(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleMaterial(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        ComponentLoader::LoadMaterial(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleVideoMap(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        // VIDEO_MAP scaleX scaleY offsetX offsetY
        float sx = 1.0f, sy = 1.0f, ox = 0.0f, oy = 0.0f;
        ss >> sx >> sy >> ox >> oy;
        
        auto& mat = scene.registry.get_or_emplace<MaterialComponent>(entity);
        mat.uvScale = glm::vec2(sx, sy);
        mat.uvOffset = glm::vec2(ox, oy);
    }

    // Lighting
    void ComponentCommandHandler::HandleDirectionalLight(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        float dx, dy, dz, r, g, b, i;
        ss >> dx >> dy >> dz >> r >> g >> b >> i;
        auto& l = scene.registry.emplace<DirectionalLightComponent>(entity);
        l.direction = glm::vec3(dx, dy, dz);
        l.color = glm::vec3(r, g, b);
        l.intensity = i;

        // Sync Transform Rotation
        if (scene.registry.all_of<TransformComponent>(entity))
        {
            auto& trans = scene.registry.get<TransformComponent>(entity);
            // Default forward is (0,0,-1). We want to rotate (0,0,-1) to light direction.
            // But light direction in "LIGHT_DIR" is FROM source or TO source?
            // "LIGHT_DIR -0.5 -1.0 -0.5" looks like direction of the light rays (downwards).
            // So we want Local Forward (0,0,-1) to align with this direction.
            
            glm::vec3 direction = glm::normalize(l.direction);
            
            // Handle edge case where direction is exactly up or down
            if (glm::abs(glm::dot(direction, glm::vec3(0,0,1))) > 0.999f) {
                 // Use a different up vector if parallel to Z
                 trans.rotation = glm::quatLookAt(direction, glm::vec3(0,1,0)); 
            } else {
                 // Standard lookAt. Note: quatLookAt expects direction to be "forward"
                 // Our components use (0,0,-1) as forward.
                 // glm::quatLookAt creates a rotation that looks in 'direction' with 'up'.
                 // Let's verify if this matches trans.rotation * (0,0,-1).
                 // GLM quatLookAt usually creates transform from Camera perspective (looking down -Z).
                 // So if we pass 'direction', the resulting rotation R * (0,0,-1) = direction.
                 // We will need a stable up vector.
                 trans.rotation = glm::quatLookAt(direction, glm::vec3(0,1,0));
            }
        }

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
        
        int primaryFlag = 0;
        int activeFlag = 1;
        if (ss >> primaryFlag)
        {
            l.isPrimary = (primaryFlag != 0);
            if (ss >> activeFlag)
            {
                l.active = (activeFlag != 0);
            }
        }
    }

    void ComponentCommandHandler::HandlePointLight(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        float r, g, b, i, rad;
        ss >> r >> g >> b >> i >> rad;
        auto& l = scene.registry.emplace<PointLightComponent>(entity);
        l.color = glm::vec3(r, g, b);
        l.intensity = i;
        l.radius = rad;

        float c, lin, quad;
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
        
        int primaryFlag = 0;
        int activeFlag = 1;
        std::string temp;
        if (ss >> temp)
        {
            primaryFlag = std::stoi(temp);
            l.isPrimary = (primaryFlag != 0);
            if (ss >> activeFlag)
            {
                l.active = (activeFlag != 0);
            }
        }
    }

    void ComponentCommandHandler::HandleSpotLight(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        float r, g, b, i, cut, outer;
        ss >> r >> g >> b >> i >> cut >> outer;
        auto& l = scene.registry.emplace<SpotLightComponent>(entity);
        l.color = glm::vec3(r, g, b);
        l.intensity = i;
        l.cutOff = glm::cos(glm::radians(cut));
        l.outerCutOff = glm::cos(glm::radians(outer));

        float c, lin, quad;
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
        
        int primaryFlag = 0;
        int activeFlag = 1;
        std::string temp;
        if (ss >> temp)
        {
            primaryFlag = std::stoi(temp);
            l.isPrimary = (primaryFlag != 0);
            if (ss >> activeFlag)
            {
                l.active = (activeFlag != 0);
            }
        }
    }

    // Camera
    void ComponentCommandHandler::HandleCamera(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        ComponentLoader::LoadCamera(scene, entity, ss);
    }

    // Physics
    void ComponentCommandHandler::HandleRigidBody(std::stringstream& ss, Scene& scene, entt::entity entity, 
                                                 PhysicsWorld& phys, std::ifstream& file)
    {
        PhysicsLoader::LoadRigidBody(scene, entity, ss, phys, file);
    }

    // UI
    void ComponentCommandHandler::HandleUITransform(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        ComponentLoader::LoadUITransform(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleUIRenderer(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res)
    {
        ComponentLoader::LoadUIRenderer(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleUIText(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res)
    {
        ComponentLoader::LoadUIText(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleUIAnimation(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        ComponentLoader::LoadUIAnimation(scene, entity, ss);
    }

    // Others
    void ComponentCommandHandler::HandleSkyboxRenderer(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res)
    {
        ComponentLoader::LoadSkyboxRenderer(scene, entity, ss, res);
    }

    void ComponentCommandHandler::HandleScript(std::stringstream& ss, Scene& scene, entt::entity entity, Application* app)
    {
        ComponentLoader::LoadScript(scene, entity, ss, app);
    }

    void ComponentCommandHandler::HandleAudioSource(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        ComponentLoader::LoadAudioSource(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleVideoPlayer(std::stringstream& ss, Scene& scene, entt::entity entity)
    {
        ComponentLoader::LoadVideoPlayer(scene, entity, ss);
    }

    void ComponentCommandHandler::HandleParticleEmitter(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res)
    {
        ComponentLoader::LoadParticleEmitter(scene, entity, ss, res);
    }
}
