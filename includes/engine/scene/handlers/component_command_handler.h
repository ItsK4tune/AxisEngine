#pragma once

#include <sstream>
#include <string>
#include <fstream>
#include <entt/entt.hpp>

// Forward declarations
class Scene;
class ResourceManager;
class PhysicsWorld;
class Application;

namespace SceneHandlers
{
    class ComponentCommandHandler
    {
    public:
        // Rendering
        static void HandleRenderer(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res);
        static void HandleAnimator(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res);
        static void HandleMaterial(std::stringstream& ss, Scene& scene, entt::entity entity);
        static void HandleVideoMap(std::stringstream& ss, Scene& scene, entt::entity entity);
        
        // Lighting
        static void HandleDirectionalLight(std::stringstream& ss, Scene& scene, entt::entity entity);
        static void HandlePointLight(std::stringstream& ss, Scene& scene, entt::entity entity);
        static void HandleSpotLight(std::stringstream& ss, Scene& scene, entt::entity entity);
        
        // Camera
        static void HandleCamera(std::stringstream& ss, Scene& scene, entt::entity entity);
        
        // Physics
        static void HandleRigidBody(std::stringstream& ss, Scene& scene, entt::entity entity, 
                                   PhysicsWorld& phys, std::ifstream& file);
        
        // UI
        static void HandleUITransform(std::stringstream& ss, Scene& scene, entt::entity entity);
        static void HandleUIRenderer(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res);
        static void HandleUIText(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res);
        static void HandleUIAnimation(std::stringstream& ss, Scene& scene, entt::entity entity);
        
        // Others
        static void HandleSkyboxRenderer(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res);
        static void HandleScript(std::stringstream& ss, Scene& scene, entt::entity entity, Application* app);
        static void HandleAudioSource(std::stringstream& ss, Scene& scene, entt::entity entity);
        static void HandleVideoPlayer(std::stringstream& ss, Scene& scene, entt::entity entity);
        static void HandleParticleEmitter(std::stringstream& ss, Scene& scene, entt::entity entity, ResourceManager& res);
    };
}
