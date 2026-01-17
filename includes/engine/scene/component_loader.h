#pragma once

#include <scene/scene.h>
#include <resource/resource_manager.h>
#include <audio/sound_manager.h>
#include <app/application.h>
#include <sstream>

class ComponentLoader
{
public:
    static void LoadRenderer(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res);
    static void LoadAnimator(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res);
    static void LoadLightDir(Scene& scene, entt::entity entity, std::stringstream& ss);
    static void LoadLightPoint(Scene& scene, entt::entity entity, std::stringstream& ss);
    static void LoadLightSpot(Scene& scene, entt::entity entity, std::stringstream& ss);
    
    // UI
    static void LoadUITransform(Scene& scene, entt::entity entity, std::stringstream& ss);
    static void LoadUIRenderer(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res);
    static void LoadUIText(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res);
    static void LoadUIAnimation(Scene& scene, entt::entity entity, std::stringstream& ss);
    
    static void LoadSkyboxRenderer(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res);
    static void LoadScript(Scene& scene, entt::entity entity, std::stringstream& ss, Application* app);
    
    static void LoadAudioSource(Scene& scene, entt::entity entity, std::stringstream& ss);
    static void LoadVideoPlayer(Scene& scene, entt::entity entity, std::stringstream& ss);
    static void LoadParticleEmitter(Scene& scene, entt::entity entity, std::stringstream& ss, ResourceManager& res);
    static void LoadMaterial(Scene& scene, entt::entity entity, std::stringstream& ss);
    static void LoadCamera(Scene& scene, entt::entity entity, std::stringstream& ss);
};
