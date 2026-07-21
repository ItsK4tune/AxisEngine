#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <resource/logic/resource_manager.h>
#include <render/logic/particle_emitter.h>
#include <render/logic/transient_buffer_ring.h>
#include <scene/logic/scene.h>

class IGraphicsContext;

class ParticleSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem, public IOptimizationConfigurable
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
        return 85;
    }
    std::string GetName() const override
    {
        return "ParticleSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::Update | SystemCategory::RenderTransparent;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void Update(Scene& scene, float dt) override;
    void RenderTransparentPass(Scene& scene, int width, int height, float alpha) override;
    void RenderParticles(Scene& scene, int width, int height, float alpha);
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;
    void SetSpawnBudget(bool enabled, unsigned int maxPerFrame)
    {
        m_SpawnBudgetEnabled = enabled;
        m_MaxSpawnPerFrame = maxPerFrame > 0 ? maxPerFrame : 1;
    }
    void SetParticleBatching(bool enabled) { m_ParticleBatchingEnabled = enabled; }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    IGraphicsContext* m_Context = nullptr;
    unsigned int m_DefaultTexture = 0;
    unsigned int m_ParticleVAO = 0;
    unsigned int m_ParticleVBO = 0;
    std::unique_ptr<TransientBufferRing> m_ParticleInstanceUpload;
    struct ParticleBatch
    {
        std::shared_ptr<Shader> shader;
        unsigned int texture = 0;
        std::vector<ParticleInstanceData> instances;
    };
    std::vector<ParticleBatch> m_ParticleBatches;
    size_t m_ActiveParticleBatchCount = 0;
    bool m_Enabled = true;
    bool m_SpawnBudgetEnabled = true;
    bool m_ParticleBatchingEnabled = true;
    unsigned int m_MaxSpawnPerFrame = 4096;
    std::vector<entt::entity> m_DestroyScratch;
};
