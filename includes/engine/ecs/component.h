#pragma once

#include <ecs/components/animation_component.h>
#include <ecs/components/audio_component.h>
#include <ecs/components/camera_component.h>
#include <ecs/components/info_component.h>
#include <ecs/components/light_components.h>
#include <ecs/components/particle_component.h>
#include <ecs/components/physics_components.h>
#include <ecs/components/render_components.h>
#include <ecs/components/script_component.h>
#include <ecs/components/transform_component.h>
#include <ecs/components/ui_components.h>
#include <ecs/components/video_component.h>
#include <entt/entt.hpp>
#include <functional>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

struct StreamingComponent
{
    std::string modelPath;
    bool isStatic = false;
    float loadDistance = 100.0f;
    bool isRequested = false;
};
