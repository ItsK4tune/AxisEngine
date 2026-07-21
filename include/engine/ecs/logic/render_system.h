#pragma once

#include <core/type/app_config.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_update_system.h>
#include <ecs/unit/render_components.h>
#include <render/interface/i_query_manager.h>
#include <render/type/graphics_types.h>
#include <render/unit/render_command.h>
#include <scene/logic/scene.h>
#include <glm/glm.hpp>
#include <vector>

class IGraphicsContext;
class ResourceManager;
class Shader;
class ShadowRenderer;
class MaterialRenderer;

class RenderSystem : public IUpdateSystem, public IECSSystem
{
public:
    RenderSystem();
    virtual ~RenderSystem();

    void Initialize() override;
    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    int GetPriority() const override
    {
        return 100;
    }
    std::string GetName() const override
    {
        return "RenderSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::Update;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }

    void Update(Scene& scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    void SetFilterLayerMask(uint32_t mask);
    void SetFaceCulling(bool enabled, CullMode mode = CullMode::Back);
    void SetDepthTest(bool enabled, CompareFunc func = CompareFunc::Less);
    void SetInstanceBatching(bool enable);
    void SetFrustumCulling(bool enable);
    void SetRenderOrderEnabled(bool enable);

private:
    bool m_Enabled = true;
    class IRenderRuntimeControl* m_RenderService = nullptr;
};
