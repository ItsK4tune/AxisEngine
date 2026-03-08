#include <render/logic/particle_emitter.h>

#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_draw_context.h>
#include <core/logic/logger.h>
#include <render/type/graphics_types.h>

IBufferManager* ParticleEmitter::s_BufferManager = nullptr;
ITextureManager* ParticleEmitter::s_TextureManager = nullptr;
IDrawContext* ParticleEmitter::s_DrawContext = nullptr;

void ParticleEmitter::SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager, IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_TextureManager = &textureManager;
    s_DrawContext = &drawContext;
}

IBufferManager& ParticleEmitter::GetBufferManager()
{
    if (!s_BufferManager) {
        LOGGER_ERROR("ParticleEmitter") << "BufferManager not set!";
        throw std::runtime_error("BufferManager not set in ParticleEmitter");
    }
    return *s_BufferManager;
}

ITextureManager& ParticleEmitter::GetTextureManager()
{
    if (!s_TextureManager) {
        LOGGER_ERROR("ParticleEmitter") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in ParticleEmitter");
    }
    return *s_TextureManager;
}

IDrawContext& ParticleEmitter::GetDrawContext()
{
    if (!s_DrawContext) {
        LOGGER_ERROR("ParticleEmitter") << "DrawContext not set!";
        throw std::runtime_error("DrawContext not set in ParticleEmitter");
    }
    return *s_DrawContext;
}

ParticleEmitter::ParticleEmitter() : m_VAO(0), m_VBO(0), m_instanceVBO(0)
{

}

ParticleEmitter::~ParticleEmitter()
{
    if (s_BufferManager)
    {
        try {
            if (m_VAO != 0) s_BufferManager->DeleteVertexArrays(1, &m_VAO);
            if (m_VBO != 0) s_BufferManager->DeleteBuffers(1, &m_VBO);
            if (m_instanceVBO != 0) s_BufferManager->DeleteBuffers(1, &m_instanceVBO);
        } catch (...) {
            LOGGER_ERROR("ParticleEmitter") << "Destructor: CRASH during buffer deletion";
        }
    }
}

ParticleEmitter::ParticleEmitter(ParticleEmitter &&other) noexcept
    : m_Particles(std::move(other.m_Particles)),
      m_MaxParticles(other.m_MaxParticles),
      m_LastUsedParticle(other.m_LastUsedParticle),
      m_VAO(other.m_VAO),
      m_VBO(other.m_VBO),
      m_instanceVBO(other.m_instanceVBO),
      Texture(other.Texture),
      Offset(other.Offset),
      MinVelocity(other.MinVelocity),
      MaxVelocity(other.MaxVelocity),
      StartColor(other.StartColor),
      EndColor(other.EndColor),
      StartSize(other.StartSize),
      EndSize(other.EndSize),
      LifeTime(other.LifeTime),
      StartLife(other.StartLife),
      SpawnRate(other.SpawnRate),
      Shape(other.Shape),
      m_SpawnAccumulator(other.m_SpawnAccumulator)
{
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_instanceVBO = 0;
}

ParticleEmitter &ParticleEmitter::operator=(ParticleEmitter &&other) noexcept
{
    if (this != &other)
    {
        if (s_BufferManager)
        {
            if (m_VAO != 0) s_BufferManager->DeleteVertexArrays(1, &m_VAO);
            if (m_VBO != 0) s_BufferManager->DeleteBuffers(1, &m_VBO);
            if (m_instanceVBO != 0) s_BufferManager->DeleteBuffers(1, &m_instanceVBO);
        }

        m_Particles = std::move(other.m_Particles);
        m_MaxParticles = other.m_MaxParticles;
        m_LastUsedParticle = other.m_LastUsedParticle;
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_instanceVBO = other.m_instanceVBO;
        Texture = other.Texture;

        Offset = other.Offset;
        MinVelocity = other.MinVelocity;
        MaxVelocity = other.MaxVelocity;
        StartColor = other.StartColor;
        EndColor = other.EndColor;
        StartSize = other.StartSize;
        EndSize = other.EndSize;
        LifeTime = other.LifeTime;
        StartLife = other.StartLife;
        SpawnRate = other.SpawnRate;
        Shape = other.Shape;
        m_SpawnAccumulator = other.m_SpawnAccumulator;

        other.m_VAO = 0;
        other.m_VBO = 0;
        other.m_instanceVBO = 0;
    }
    return *this;
}

void ParticleEmitter::Init(unsigned int maxParticles)
{
    if (!s_BufferManager) return;
    m_MaxParticles = maxParticles;
    m_Particles.resize(m_MaxParticles);

    auto& bm = GetBufferManager();

    float quadVertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f};

    m_VAO = bm.GenVertexArray();
    m_VBO = bm.GenBuffer();
    m_instanceVBO = bm.GenBuffer();

    bm.BindVertexArray(m_VAO);

    bm.BindBuffer(BufferType::ArrayBuffer, m_VBO);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), quadVertices, BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 5 * sizeof(float), (void *)0);
    bm.EnableVertexAttribArray(1);

    bm.VertexAttribPointer(1, 2, DataType::Float, false, 5 * sizeof(float), (void *)(3 * sizeof(float)));

    bm.BindBuffer(BufferType::ArrayBuffer, m_instanceVBO);
    bm.BufferData(BufferType::ArrayBuffer, m_MaxParticles * sizeof(ParticleInstanceData), nullptr, BufferUsage::StreamDraw);

    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 4, DataType::Float, false, sizeof(ParticleInstanceData), (void *)offsetof(ParticleInstanceData, color));
    bm.VertexAttribDivisor(2, 1);

    bm.EnableVertexAttribArray(3);
    bm.VertexAttribPointer(3, 3, DataType::Float, false, sizeof(ParticleInstanceData), (void *)offsetof(ParticleInstanceData, offset));
    bm.VertexAttribDivisor(3, 1);

    bm.EnableVertexAttribArray(4);
    bm.VertexAttribPointer(4, 1, DataType::Float, false, sizeof(ParticleInstanceData), (void *)offsetof(ParticleInstanceData, scale));
    bm.VertexAttribDivisor(4, 1);

    bm.BindVertexArray(0);
}

#include <random>

float RandomFloat()
{
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    return distribution(generator);
}

float RandomFloat(float min, float max)
{
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator);
}

void ParticleEmitter::Update(float dt, const glm::vec3 &offset)
{
    m_SpawnAccumulator += dt;
    float rate = (SpawnRate > 0.0f) ? (1.0f / SpawnRate) : 0.0f;

    while (m_SpawnAccumulator >= rate && rate > 0.0001f)
    {
        int unusedParticle = FirstUnusedParticle();
        RespawnParticle(m_Particles[unusedParticle], offset);
        m_SpawnAccumulator -= rate;
    }

    for (unsigned int i = 0; i < m_MaxParticles; ++i)
    {
        Particle &p = m_Particles[i];
        p.Life -= dt;
        if (p.Life > 0.0f)
        {
            p.Position += p.Velocity * dt;

            float t = 1.0f - (p.Life / p.StartLife);
            p.Color = StartColor * (1.0f - t) + EndColor * t;
            p.Size = StartSize * (1.0f - t) + EndSize * t;
        }
    }
}

void ParticleEmitter::Render(Shader *shader)
{
    if (!s_TextureManager || !s_BufferManager || !s_DrawContext) return;
    auto& tm = GetTextureManager();
    auto& bm = GetBufferManager();
    auto& dc = GetDrawContext();

    if (Texture)
    {
        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, Texture->id);
        shader->setInt("sprite", 0);
    }

    std::vector<ParticleInstanceData> instanceData;
    instanceData.reserve(m_MaxParticles);

    for (const auto &particle : m_Particles)
    {
        if (particle.Life > 0.0f)
        {
            ParticleInstanceData data;
            data.color = particle.Color;
            data.offset = particle.Position;
            data.scale = particle.Size;
            instanceData.push_back(data);
        }
    }

    if (instanceData.empty())
        return;

    bm.BindBuffer(BufferType::ArrayBuffer, m_instanceVBO);
    bm.BufferSubData(BufferType::ArrayBuffer, 0, instanceData.size() * sizeof(ParticleInstanceData), instanceData.data());
    bm.BindBuffer(BufferType::ArrayBuffer, 0);

    bm.BindVertexArray(m_VAO);
    dc.DrawArraysInstanced(Primitive::Triangles, 0, 6, static_cast<unsigned int>(instanceData.size()));
    bm.BindVertexArray(0);
}

unsigned int ParticleEmitter::FirstUnusedParticle()
{
    for (unsigned int i = m_LastUsedParticle; i < m_MaxParticles; ++i)
    {
        if (m_Particles[i].Life <= 0.0f)
        {
            m_LastUsedParticle = i;
            return i;
        }
    }
    for (unsigned int i = 0; i < m_LastUsedParticle; ++i)
    {
        if (m_Particles[i].Life <= 0.0f)
        {
            m_LastUsedParticle = i;
            return i;
        }
    }
    return 0;
}

void ParticleEmitter::RespawnParticle(Particle &particle, const glm::vec3 &offset)
{
    float rX = RandomFloat(-0.5f, 0.5f);
    float rY = RandomFloat(-0.5f, 0.5f);
    float rZ = RandomFloat(-0.5f, 0.5f);

    particle.Position = offset + glm::vec3(rX, rY, rZ) * 0.1f;

    particle.StartLife = LifeTime;
    particle.Life = LifeTime;
    particle.Size = StartSize;
    particle.Color = StartColor;

    float vFactor = RandomFloat();
    glm::vec3 baseVel = MinVelocity + (MaxVelocity - MinVelocity) * vFactor;

    if (Shape == EmissionShape::DIRECTIONAL)
    {
        particle.Velocity = baseVel;
    }
    else if (Shape == EmissionShape::CONE)
    {
        float angleX = RandomFloat(-0.5f, 0.5f);
        float angleZ = RandomFloat(-0.5f, 0.5f);
        glm::vec3 dir = glm::normalize(glm::vec3(angleX, 1.0f, angleZ));
        float speed = glm::length(baseVel);
        particle.Velocity = dir * speed;
    }
    else if (Shape == EmissionShape::FIGURE_EIGHT)
    {
        static float timeAcc = 0.0f;
        timeAcc += 0.1f;
        float x = sin(timeAcc);
        float z = sin(timeAcc / 2.0f);
        particle.Velocity = glm::vec3(x, 1.0f, z) * glm::length(baseVel);
    }
}