#pragma once

#include <axis/common.h>

// --- Entity Management ---
#include <ecs/manager/entity_manager.h>
#include <ecs/logic/entity_builder.h>

// --- Core Components ---
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>

// --- Specialized Components ---
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>

// --- Core Systems ---
#include <ecs/logic/render_system.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/script_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/ui_system.h>
#include <ecs/logic/video_system.h>
#include <ecs/logic/skybox_system.h>
#include <ecs/logic/streaming_system.h>
#include <ecs/logic/transform_system.h>
