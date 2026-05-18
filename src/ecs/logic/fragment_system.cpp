#include <engine/ecs/logic/fragment_system.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <engine/ecs/logic/transform_system.h>
#include <engine/ecs/unit/core_components.h>
#include <engine/ecs/unit/fragment_component.h>
#include <engine/scene/logic/fragment_loader.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene_manager.h>
#include <functional>

REGISTER_SYSTEM(FragmentSystem)

FragmentSystem::FragmentSystem()
{
}

void FragmentSystem::Update(Scene& scene, float dt)
{
    auto& sl = ServiceLocator::Instance();
    auto* resPtr = sl.Resolve<ResourceManager>();
    if (!resPtr)
        return;
    auto& res = *resPtr;

    auto* phys = sl.Resolve<IPhysicsWorld>();

    auto* audioSvc = sl.Resolve<AudioService>();

    auto view = scene.registry.view<FragmentComponent>();

    // Resolve SceneManager once ΓÇö needed for both cleanup and re-registration
    auto* sceneMgr = sl.Resolve<SceneManager>();

    for (auto entity : view)
    {
        auto& comp = view.get<FragmentComponent>(entity);
        if (comp.instantiated)
            continue;

        // Clear existing children before re-instantiating (recursive)
        if (auto* h = scene.registry.try_get<HierarchyComponent>(entity))
        {
            auto childrenCopy = h->children;

            std::function<void(entt::entity)> destroyRecursive = [&](entt::entity e) {
                if (auto* hc = scene.registry.try_get<HierarchyComponent>(e))
                {
                    auto cc = hc->children;
                    for (auto c : cc) destroyRecursive(c);
                }
                if (sceneMgr)
                    sceneMgr->RemoveEntity(e);
                scene.registry.destroy(e);
            };

            for (auto child : childrenCopy)
            {
                destroyRecursive(child);
            }
            h->children.clear();
        }

        auto asset = res.GetFragment(comp.path);
        if (!asset)
        {
            LOGGER_ERROR("FragmentSystem") << "Failed to load fragment asset: " << comp.path;
            comp.instantiated = true;
            continue;
        }

        YAMLNode virtualOverrideNode;
        if (!comp.overrides.empty())
        {
            virtualOverrideNode.children = YAMLParser::ParseString(comp.overrides);
        }

        LOGGER_INFO("FragmentSystem") << "Instantiating fragment '" << comp.path << "' on entity " << (uint32_t)entity;

        std::string parentSceneName = "";
        bool parentActive = true;
        if (auto* info = scene.registry.try_get<InfoComponent>(entity))
        {
            parentSceneName = info->sceneName;
            parentActive = info->isActive;
        }

        auto instantiated =
            FragmentLoader::Instantiate(*asset, scene, entity, res, phys, audioSvc, &virtualOverrideNode);

        for (auto const& [name, e] : instantiated)
        {
            if (auto* info = scene.registry.try_get<InfoComponent>(e))
            {
                if (!parentSceneName.empty())
                    info->sceneName = parentSceneName;
                info->isActive = parentActive;
            }
            if (sceneMgr && !parentSceneName.empty())
            {
                sceneMgr->AddEntity(e, parentSceneName);
            }
        }

        comp.instantiated = true;

        // Force immediate transform update to prevent (0,0,0) clustering
        auto* ts = sl.Resolve<TransformSystem>();
        if (ts)
        {
            ts->m_IsLinearTransformsDirty = true;
            ts->Update(scene, 0.0f);
        }
    }
}
