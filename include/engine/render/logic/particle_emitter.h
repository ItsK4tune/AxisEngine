#pragma once

#include <resource/unit/mesh.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class IBufferManager;
class IDrawContext;
class ITextureManager;
class TransientBufferRing;


struct Particle
{
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec4 Color;
    float Life;
    float StartLife;
    float Size;
    float CameraDistance;

    bool operator<(const Particle& that) const
    {
        return this->CameraDistance > that.CameraDistance;
    }
};

struct ParticleInstanceData
{
    glm::vec4 color;
    glm::vec3 offset;
    float scale;
};

class ParticleEmitter
{
public:
    ParticleEmitter();
    ~ParticleEmitter();

    ParticleEmitter(const ParticleEmitter&) = delete;
    ParticleEmitter& operator=(const ParticleEmitter&) = delete;

    ParticleEmitter(ParticleEmitter&& other) noexcept;
    ParticleEmitter& operator=(ParticleEmitter&& other) noexcept;

    void Initialize(unsigned int maxParticles = 500);
    void Update(float dt, const glm::vec3& offset = glm::vec3(0.0f), bool spawn = true);
    void Render(Shader* shader);

    unsigned int GetActiveParticleCount() const;
    const std::vector<ParticleInstanceData>& GetInstanceData() const { return m_InstanceData; }
    void SetSpawnBudget(bool enabled, unsigned int maxPerFrame)
    {
        m_SpawnBudgetEnabled = enabled;
        m_MaxSpawnPerFrame = maxPerFrame > 0 ? maxPerFrame : 1;
    }

    enum class EmissionShape
    {
        DIRECTIONAL,
        CONE,
        FIGURE_EIGHT
    };

    glm::vec3 MinVelocity = glm::vec3(-0.1f, 1.0f, -0.1f);
    glm::vec3 MaxVelocity = glm::vec3(0.1f, 4.0f, 0.1f);
    glm::vec4 StartColor = glm::vec4(1.0f);
    glm::vec4 EndColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    float StartSize = 1.0f;
    float EndSize = 0.0f;
    float LifeTime = 1.0f;
    float SpawnRate = 10.0f;
    EmissionShape Shape = EmissionShape::DIRECTIONAL;

    std::shared_ptr<Texture> texture = nullptr;

    static void SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext);
    static void ClearManagers();

private:
    std::vector<Particle> m_Particles;
    std::vector<unsigned int> m_ActiveIndices;
    std::vector<unsigned int> m_FreeIndices;
    std::vector<ParticleInstanceData> m_InstanceData;
    unsigned int m_MaxParticles = 0;
    float m_SpawnAccumulator = 0.0f;
    float m_EmissionPhase = 0.0f;
    bool m_SpawnBudgetEnabled = true;
    unsigned int m_MaxSpawnPerFrame = 4096;

    unsigned int m_VAO, m_VBO;
    unsigned int m_instanceVBO;
    std::unique_ptr<TransientBufferRing> m_InstanceUpload;

    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;

    void RespawnParticle(Particle& particle, const glm::vec3& offset);

    static IBufferManager& GetBufferManager();
    static ITextureManager& GetTextureManager();
    static IDrawContext& GetDrawContext();
};
