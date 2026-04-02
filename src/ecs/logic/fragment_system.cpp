#include <engine/ecs/logic/fragment_system.h>
#include <engine/ecs/unit/fragment_component.h>
#include <engine/scene/logic/fragment_loader.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>

REGISTER_SYSTEM(FragmentSystem)

FragmentSystem::FragmentSystem() {}

void FragmentSystem::Update(Scene& scene, float dt)
{
    auto& sl = ServiceLocator::Instance();
    auto* resPtr = sl.Resolve<ResourceManager>();
    if (!resPtr) return;
    auto& res = *resPtr;

    auto* phys = sl.Resolve<IPhysicsWorld>();
    
    auto* audioSvc = sl.Resolve<AudioService>();
    if (!audioSvc) return;

    auto view = scene.registry.view<FragmentComponent>();
    
    for (auto entity : view)
    {
        auto& comp = view.get<FragmentComponent>(entity);
        if (comp.instantiated) continue;

        auto asset = res.GetFragment(comp.path);
        if (!asset)
        {
            LOGGER_ERROR("FragmentSystem") << "Failed to load fragment asset: " << comp.path;
            comp.instantiated = true; 
            continue;
        }

        YAMLNode* overrideNode = comp.overrideNode.GetChild("Override");
        
        LOGGER_INFO("FragmentSystem") << "Instantiating fragment '" << comp.path << "' on entity " << (uint32_t)entity;
        
        FragmentLoader::Instantiate(*asset, scene, entity, res, phys, *audioSvc, overrideNode);
        
        comp.instantiated = true;
    }
}
