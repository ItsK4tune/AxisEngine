#pragma once

#include <engine/resource/type/fragment_asset.h>
#include <engine/scene/logic/scene.h>
#include <resource/logic/resource_manager.h>
#include <audio/logic/audio_service.h>
#include <physics/interface/i_physics_world.h>
#include <map>
#include <string>

class FragmentLoader {
public:
    static std::map<std::string, entt::entity> Instantiate(
        const FragmentAsset& asset, 
        Scene& scene, 
        entt::entity parent, 
        ResourceManager& res, 
        IPhysicsWorld* phys, 
        AudioService& sound,
        const YAMLNode* overrideNode = nullptr
    );
};
