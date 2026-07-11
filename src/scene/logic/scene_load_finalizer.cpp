#include <scene/logic/scene_load_finalizer.h>

#include <core/logic/config_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <scene/logic/scene_post_load_fixup.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_validator.h>
#include <scene/type/scene_record.h>
#include <algorithm>

namespace SceneHandlers
{
bool SceneLoadFinalizer::Finalize(Scene& scene, SceneLoadResult& result, IPhysicsWorld* physics,
                                  const std::map<entt::entity, std::vector<std::string>>& deferredChildren,
                                  const SceneLoadFinalizeOptions& options)
{
    ScenePostLoadFixup::ResolveParentChildRelationships(scene, deferredChildren);

    SceneValidationOptions validationOptions;
    validationOptions.entityScope = result.entities;
    validationOptions.deepValidation = options.deepValidation;
    if (auto* configManager = ServiceLocator::Instance().Resolve<ConfigManager>())
    {
        const auto& shadow = configManager->GetConfig().shadow;
        validationOptions.validateRenderCapabilities = true;
        if (!shadow.shadowsEnabled || shadow.shadowMode == 0)
        {
            validationOptions.renderCapabilities.directionalShadowLimit = 0;
            validationOptions.renderCapabilities.pointShadowLimit = 0;
            validationOptions.renderCapabilities.spotShadowLimit = 0;
        }
        else
        {
            validationOptions.renderCapabilities.singleDirectionalShadow = shadow.shadowMode != 2;
        }
    }

    result.validation = SceneValidator::Validate(scene, validationOptions);
    SceneValidator::LogIssues(result.validation);
    if (result.validation.HasFatalErrors())
    {
        for (auto it = result.entities.rbegin(); it != result.entities.rend(); ++it)
        {
            if (scene.IsValid(*it))
                scene.Destroy(*it);
        }
        result.entities.clear();
        return false;
    }

    if (options.ensureFallbackCamera)
        result.usedFallbackCamera = ScenePostLoadFixup::EnsureFallbackCamera(scene);
    if (options.initializePhysicsBindings)
        ScenePostLoadFixup::InitializePhysicsBindings(scene, physics, result.entities);
    return true;
}
}  // namespace SceneHandlers
