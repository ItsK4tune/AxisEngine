#pragma once

#include <scene/type/scene_validation.h>

struct Scene;

namespace SceneHandlers
{
class SceneValidator
{
public:
    static SceneValidationResult Validate(const Scene& scene,
                                          const SceneValidationOptions& options = SceneValidationOptions{});
    static void LogIssues(const SceneValidationResult& result);
};
}  // namespace SceneHandlers
