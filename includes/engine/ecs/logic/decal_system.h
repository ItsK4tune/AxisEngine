#pragma once

#include <ecs/interface/i_system.h>
#include <render/logic/shader.h>
#include <render/type/graphics_types.h>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <entt/entt.hpp>
#include <resource/interface/i_shader_library.h>

class IGraphicsContext;

class DecalSystem : public ISystem
{
public:
    void Initialize(EngineContext ctx) override;
    void Shutdown() override {}
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 82; } // After Terrain
    std::string GetName() const override { return "DecalSystem"; }

    void Update(Scene &scene, float dt) override;
    void Render(Scene &scene) override;

    // API
    uint32_t LoadDecalTexture(const std::string& path);
    void FlushDecals(Scene& scene);
    
    // Set global tags that decals can stick to if they don't have specific ones
    void SetAllowedTags(const std::vector<std::string>& tags) { m_AllowedTags = tags; }
    uint32_t GetTagBit(const std::string& tag);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;
    void OnDecalConstruct(entt::registry& registry, entt::entity entity);

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
    std::shared_ptr<Shader> m_DecalShader;
    std::shared_ptr<Shader> m_ForwardShader;
    
    GpuHandle m_CubeVAO = 0;
    GpuHandle m_CubeVBO = 0;
    GpuHandle m_CubeEBO = 0;
    
    std::vector<std::string> m_AllowedTags;
    std::map<std::string, uint32_t> m_TagBitMap;
    unsigned int m_TagMapTexture = 0;
    std::vector<uint32_t> m_TagBuffer;
    uint32_t m_NextOrder = 1;

    void InitCubeMesh();
    void UpdateTagMap(Scene& scene);
    uint32_t GetBitmask(const std::vector<std::string>& tags);
};
