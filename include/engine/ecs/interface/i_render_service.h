#pragma once
#include <core/type/render_path.h>
#include <ecs/interface/i_base_system.h>
#include <render/type/graphics_types.h>
#include <glm/glm.hpp>

class RenderQueue;
class StaticBatchManager;
class ShadowRenderer;
class MaterialRenderer;
class Shader;
struct Scene;
struct RenderItem;

#include <ecs/interface/i_camera_service.h>
#include <ecs/interface/i_ibl_service.h>
#include <ecs/interface/i_render_queue_service.h>
#include <ecs/interface/i_render_state_service.h>

enum class AntiAliasingMode
{
    NONE = 0,
    FXAA = 1,
    TAA = 2
};

class IRenderService : virtual public IBaseSystem,
                       public ICameraService,
                       public IRenderStateService,
                       public IRenderQueueService,
                       public IIBLService
{
public:
    virtual ~IRenderService() = default;
};
