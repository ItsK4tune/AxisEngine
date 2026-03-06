#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <rendering/core/shader.h>
#include <rendering/geometry/mesh.h>
#include <memory>
#include <vector>

class IBufferManager;
class ITextureManager;
class IDrawContext;

struct Particle
{
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec4 Color;
    float Life;
    float StartLife;
    float Size;
    float CameraDistance;

    bool operator<(const Particle &that) const
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

    ParticleEmitter(const ParticleEmitter &) = delete;
    ParticleEmitter &operator=(const ParticleEmitter &) = delete;

    ParticleEmitter(ParticleEmitter &&other) noexcept;
    ParticleEmitter &operator=(ParticleEmitter &&other) noexcept;

    void Init(unsigned int maxParticles = 500);
    void Update(float dt, const glm::vec3 &offset = glm::vec3(0.0f));
    void Render(Shader *shader);

    enum class EmissionShape
    {
        DIRECTIONAL,
        CONE,
        FIGURE_EIGHT
    };

    glm::vec3 Offset = glm::vec3(0.0f);
    glm::vec3 MinVelocity = glm::vec3(-0.1f, 1.0f, -0.1f);
    glm::vec3 MaxVelocity = glm::vec3(0.1f, 4.0f, 0.1f);
    glm::vec4 StartColor = glm::vec4(1.0f);
    glm::vec4 EndColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    float StartSize = 1.0f;
    float EndSize = 0.0f;
    float LifeTime = 1.0f;
    float StartLife = 1.0f;
    float SpawnRate = 10.0f;
    EmissionShape Shape = EmissionShape::DIRECTIONAL;

    std::shared_ptr<Texture> texture = nullptr;

    static void SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext);

private:
    std::vector<Particle> m_Particles;
    unsigned int m_MaxParticles;
    unsigned int m_LastUsedParticle = 0;
    float m_SpawnAccumulator = 0.0f;

    unsigned int m_VAO, m_VBO;
    unsigned int m_instanceVBO;

    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;

    unsigned int FirstUnusedParticle();
    void RespawnParticle(Particle &particle, const glm::vec3 &offset);

    static IBufferManager& GetBufferManager();
    static ITextureManager& GetTextureManager();
    static IDrawContext& GetDrawContext();
};
