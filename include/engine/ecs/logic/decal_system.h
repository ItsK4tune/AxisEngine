#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <render/type/graphics_types.h>
#include <resource/interface/i_shader_library.h>
#include <resource/unit/shader.h>
#include <scene/logic/scene.h>
#include <entt/entt.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

class IGraphicsContext;

class DecalSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override;

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
        return 82;
    }
    std::string GetName() const override
    {
        return "DecalSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderMain | SystemCategory::RenderAlpha | SystemCategory::Update;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void RenderAlphaPass(Scene& scene, int width, int height, float alpha) override;
    void Update(Scene& scene, float dt) override;
    void Render(Scene& scene) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;
    void OnDecalConstruct(entt::registry& registry, entt::entity entity);

private:
    bool m_Enabled = true;
    std::shared_ptr<Shader> m_DecalShader;
    std::shared_ptr<Shader> m_ForwardShader;

    std::map<std::string, uint32_t> m_TagBitMap;
    unsigned int m_TagMapTexture = 0;
    std::vector<uint32_t> m_TagBuffer;
    uint32_t m_NextOrder = 1;
    Scene* m_BoundScene = nullptr;

    class IGeometryService* m_GeoService = nullptr;
    class IRenderService* m_RenderService = nullptr;
    class IGraphicsContext* m_GraphicsContext = nullptr;

    void RenderDecals(Scene& scene, bool isDeferred);
    void UpdateTagMap(Scene& scene);
    uint32_t GetTagBit(const std::string& tag);
    uint32_t GetBitmask(const std::vector<std::string>& tags);
};
