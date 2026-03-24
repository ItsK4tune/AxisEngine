#pragma once

#include <resource/unit/shader.h>
#include <render/type/graphics_types.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <entt/entt.hpp>
#include <resource/interface/i_shader_library.h>
#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <scene/logic/scene.h>

class IGraphicsContext;

class DecalSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override {}
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 82; }
    std::string GetName() const override { return "DecalSystem"; }
    void RenderAlpha(Scene &scene, int width, int height, float alpha) override;

    void Update(Scene &scene, float dt) override;
    void Render(Scene &scene) override;


    uint32_t LoadDecalTexture(const std::string& path);
    void FlushDecals(Scene& scene);
    

    void SetAllowedTags(const std::vector<std::string>& tags) { m_AllowedTags = tags; }
    uint32_t GetTagBit(const std::string& tag);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;
    void OnDecalConstruct(entt::registry& registry, entt::entity entity);

private:
    bool m_Enabled = true;
    std::shared_ptr<Shader> m_DecalShader;
    std::shared_ptr<Shader> m_ForwardShader;
    
    GpuHandle m_CubeVAO = 0;
    GpuHandle m_CubeVBO = 0;
    GpuHandle m_CubeEBO = 0;
    
    GpuHandle m_QuadVAO = 0;
    GpuHandle m_QuadVBO = 0;
    GpuHandle m_QuadEBO = 0;
    
    std::vector<std::string> m_AllowedTags;
    std::map<std::string, uint32_t> m_TagBitMap;
    unsigned int m_TagMapTexture = 0;
    std::vector<uint32_t> m_TagBuffer;
    uint32_t m_NextOrder = 1;
    bool m_NextOrderIsReset = false;
    bool m_IsRenderedThisFrame = false;

    class IGeometryService* m_GeoService = nullptr;
    class IRenderService* m_RenderService = nullptr;
    class IGraphicsContext* m_GraphicsContext = nullptr;

    void InitCubeMesh();
    void InitQuadMesh();
    void UpdateTagMap(Scene& scene);
    uint32_t GetBitmask(const std::vector<std::string>& tags);
};
