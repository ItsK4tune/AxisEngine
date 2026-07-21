#include <render/logic/particle_emitter.h>
#include <core/logic/logger.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/transient_buffer_ring.h>
#include <render/type/graphics_types.h>
#include <algorithm>
#include <random>

IBufferManager* ParticleEmitter::s_BufferManager = nullptr;
ITextureManager* ParticleEmitter::s_TextureManager = nullptr;
IDrawContext* ParticleEmitter::s_DrawContext = nullptr;

void ParticleEmitter::SetManagers(IBufferManager& bufferManager, ITextureManager& textureManager,
                                  IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_TextureManager = &textureManager;
    s_DrawContext = &drawContext;
}

void ParticleEmitter::ClearManagers()
{
    s_BufferManager = nullptr;
    s_TextureManager = nullptr;
    s_DrawContext = nullptr;
}

IBufferManager& ParticleEmitter::GetBufferManager()
{
    if (!s_BufferManager)
    {
        LOGGER_ERROR("ParticleEmitter") << "BufferManager not set!";
        throw std::runtime_error("BufferManager not set in ParticleEmitter");
    }
    return *s_BufferManager;
}

ITextureManager& ParticleEmitter::GetTextureManager()
{
    if (!s_TextureManager)
    {
        LOGGER_ERROR("ParticleEmitter") << "TextureManager not set!";
        throw std::runtime_error("TextureManager not set in ParticleEmitter");
    }
    return *s_TextureManager;
}

IDrawContext& ParticleEmitter::GetDrawContext()
{
    if (!s_DrawContext)
    {
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
    m_InstanceUpload.reset();
    if (s_BufferManager)
    {
        try
        {
            if (m_VAO != 0)
                s_BufferManager->DeleteVertexArrays(1, &m_VAO);
            if (m_VBO != 0)
                s_BufferManager->DeleteBuffers(1, &m_VBO);
        }
        catch (...)
        {
            LOGGER_ERROR("ParticleEmitter") << "Destructor: CRASH during buffer deletion";
        }
    }
}

ParticleEmitter::ParticleEmitter(ParticleEmitter&& other) noexcept
    : m_Particles(std::move(other.m_Particles)),
      m_ActiveIndices(std::move(other.m_ActiveIndices)),
      m_FreeIndices(std::move(other.m_FreeIndices)),
      m_InstanceData(std::move(other.m_InstanceData)),
      m_MaxParticles(other.m_MaxParticles),
      m_VAO(other.m_VAO),
      m_VBO(other.m_VBO),
      m_instanceVBO(other.m_instanceVBO),
      m_InstanceUpload(std::move(other.m_InstanceUpload)),
      texture(other.texture),
      MinVelocity(other.MinVelocity),
      MaxVelocity(other.MaxVelocity),
      StartColor(other.StartColor),
      EndColor(other.EndColor),
      StartSize(other.StartSize),
      EndSize(other.EndSize),
      LifeTime(other.LifeTime),
      SpawnRate(other.SpawnRate),
      Shape(other.Shape),
      m_SpawnAccumulator(other.m_SpawnAccumulator),
      m_EmissionPhase(other.m_EmissionPhase),
      m_SpawnBudgetEnabled(other.m_SpawnBudgetEnabled),
      m_MaxSpawnPerFrame(other.m_MaxSpawnPerFrame)
{
    other.m_VAO = 0;
    other.m_VBO = 0;
    other.m_instanceVBO = 0;
}

ParticleEmitter& ParticleEmitter::operator=(ParticleEmitter&& other) noexcept
{
    if (this != &other)
    {
        m_InstanceUpload.reset();
        if (s_BufferManager)
        {
            if (m_VAO != 0)
                s_BufferManager->DeleteVertexArrays(1, &m_VAO);
            if (m_VBO != 0)
                s_BufferManager->DeleteBuffers(1, &m_VBO);
        }

        m_Particles = std::move(other.m_Particles);
        m_ActiveIndices = std::move(other.m_ActiveIndices);
        m_FreeIndices = std::move(other.m_FreeIndices);
        m_InstanceData = std::move(other.m_InstanceData);
        m_MaxParticles = other.m_MaxParticles;
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        m_instanceVBO = other.m_instanceVBO;
        m_InstanceUpload = std::move(other.m_InstanceUpload);
        texture = other.texture;

        MinVelocity = other.MinVelocity;
        MaxVelocity = other.MaxVelocity;
        StartColor = other.StartColor;
        EndColor = other.EndColor;
        StartSize = other.StartSize;
        EndSize = other.EndSize;
        LifeTime = other.LifeTime;
        SpawnRate = other.SpawnRate;
        Shape = other.Shape;
        m_SpawnAccumulator = other.m_SpawnAccumulator;
        m_EmissionPhase = other.m_EmissionPhase;
        m_SpawnBudgetEnabled = other.m_SpawnBudgetEnabled;
        m_MaxSpawnPerFrame = other.m_MaxSpawnPerFrame;

        other.m_VAO = 0;
        other.m_VBO = 0;
        other.m_instanceVBO = 0;
    }
    return *this;
}

void ParticleEmitter::Initialize(unsigned int maxParticles)
{
    m_InstanceUpload.reset();
    if (s_BufferManager)
    {
        if (m_VAO != 0)
            s_BufferManager->DeleteVertexArrays(1, &m_VAO);
        if (m_VBO != 0)
            s_BufferManager->DeleteBuffers(1, &m_VBO);
    }
    m_VAO = m_VBO = m_instanceVBO = 0;
    m_MaxParticles = maxParticles;
    m_Particles.resize(m_MaxParticles);
    m_ActiveIndices.clear();
    m_ActiveIndices.reserve(m_MaxParticles);
    m_FreeIndices.clear();
    m_FreeIndices.reserve(m_MaxParticles);
    for (unsigned int index = m_MaxParticles; index > 0; --index)
        m_FreeIndices.push_back(index - 1);
    m_InstanceData.clear();
    m_InstanceData.reserve(m_MaxParticles);

    if (!s_BufferManager)
        return;

    auto& bm = GetBufferManager();

    float quadVertices[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
                            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
                            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  0.5f,  0.0f, 1.0f, 1.0f};

    m_VAO = bm.GenVertexArray();
    m_VBO = bm.GenBuffer();
    m_InstanceUpload = std::make_unique<TransientBufferRing>();
    m_InstanceUpload->Initialize(bm, BufferType::ArrayBuffer,
                                 (std::max)(size_t{1}, static_cast<size_t>(m_MaxParticles)) *
                                     sizeof(ParticleInstanceData));
    m_instanceVBO = m_InstanceUpload->GetBuffer();

    bm.BindVertexArray(m_VAO);

    bm.BindBuffer(BufferType::ArrayBuffer, m_VBO);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), quadVertices, BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 5 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);

    bm.VertexAttribPointer(1, 2, DataType::Float, false, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    bm.BindBuffer(BufferType::ArrayBuffer, m_instanceVBO);

    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 4, DataType::Float, false, sizeof(ParticleInstanceData),
                           (void*)offsetof(ParticleInstanceData, color));
    bm.VertexAttribDivisor(2, 1);

    bm.EnableVertexAttribArray(3);
    bm.VertexAttribPointer(3, 3, DataType::Float, false, sizeof(ParticleInstanceData),
                           (void*)offsetof(ParticleInstanceData, offset));
    bm.VertexAttribDivisor(3, 1);

    bm.EnableVertexAttribArray(4);
    bm.VertexAttribPointer(4, 1, DataType::Float, false, sizeof(ParticleInstanceData),
                           (void*)offsetof(ParticleInstanceData, scale));
    bm.VertexAttribDivisor(4, 1);

    bm.BindVertexArray(0);
}

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

void ParticleEmitter::Update(float dt, const glm::vec3& offset, bool spawn)
{
    // Age the particles that existed at the start of the frame. Particles
    // emitted below must remain visible for at least their first frame instead
    // of immediately losing the entire frame delta from their lifetime.
    for (size_t activeIndex = 0; activeIndex < m_ActiveIndices.size();)
    {
        Particle& p = m_Particles[m_ActiveIndices[activeIndex]];
        p.Life -= dt;
        if (p.Life <= 0.0f)
        {
            m_FreeIndices.push_back(m_ActiveIndices[activeIndex]);
            m_ActiveIndices[activeIndex] = m_ActiveIndices.back();
            m_ActiveIndices.pop_back();
            continue;
        }
        p.Position += p.Velocity * dt;
        const float t = 1.0f - (p.Life / p.StartLife);
        p.Color = StartColor * (1.0f - t) + EndColor * t;
        p.Size = StartSize * (1.0f - t) + EndSize * t;
        ++activeIndex;
    }

    if (spawn)
    {
        m_SpawnAccumulator += dt;
        const float interval = SpawnRate > 0.0f ? 1.0f / SpawnRate : 0.0f;
        const unsigned int maxSpawnThisFrame =
            m_SpawnBudgetEnabled ? (std::min)(m_MaxParticles, m_MaxSpawnPerFrame) : m_MaxParticles;
        unsigned int spawned = 0;
        while (interval > 0.0f && m_SpawnAccumulator >= interval && !m_FreeIndices.empty() &&
               spawned < maxSpawnThisFrame)
        {
            const unsigned int particleIndex = m_FreeIndices.back();
            m_FreeIndices.pop_back();
            RespawnParticle(m_Particles[particleIndex], offset);
            m_ActiveIndices.push_back(particleIndex);
            m_SpawnAccumulator -= interval;
            ++spawned;
        }
        if (interval <= 0.0f || m_FreeIndices.empty() || spawned == maxSpawnThisFrame)
            m_SpawnAccumulator = interval > 0.0f ? (std::min)(m_SpawnAccumulator, interval) : 0.0f;
    }

    m_InstanceData.clear();
    for (const unsigned int particleIndex : m_ActiveIndices)
    {
        const auto& particle = m_Particles[particleIndex];
        m_InstanceData.push_back({particle.Color, particle.Position, particle.Size});
    }
}

unsigned int ParticleEmitter::GetActiveParticleCount() const
{
    return static_cast<unsigned int>(m_ActiveIndices.size());
}

void ParticleEmitter::Render(Shader* shader)
{
    if (!s_TextureManager || !s_BufferManager || !s_DrawContext || !m_InstanceUpload)
        return;
    auto& tm = GetTextureManager();
    auto& bm = GetBufferManager();
    auto& dc = GetDrawContext();

    if (texture)
    {
        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, texture->id);
        shader->setInt("u_AlbedoMap", 0);
    }

    if (m_InstanceData.empty())
        return;

    const auto slice = m_InstanceUpload->Upload(m_InstanceData.data(),
                                                m_InstanceData.size() * sizeof(ParticleInstanceData));
    m_instanceVBO = slice.buffer;
    bm.BindVertexArray(m_VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_instanceVBO);
    bm.VertexAttribPointer(2, 4, DataType::Float, false, sizeof(ParticleInstanceData),
                           reinterpret_cast<void*>(slice.offset + offsetof(ParticleInstanceData, color)));
    bm.VertexAttribPointer(3, 3, DataType::Float, false, sizeof(ParticleInstanceData),
                           reinterpret_cast<void*>(slice.offset + offsetof(ParticleInstanceData, offset)));
    bm.VertexAttribPointer(4, 1, DataType::Float, false, sizeof(ParticleInstanceData),
                           reinterpret_cast<void*>(slice.offset + offsetof(ParticleInstanceData, scale)));
    dc.DrawArraysInstanced(Primitive::Triangles, 0, 6, static_cast<unsigned int>(m_InstanceData.size()));
    m_InstanceUpload->Commit();
    bm.BindVertexArray(0);
}

void ParticleEmitter::RespawnParticle(Particle& particle, const glm::vec3& offset)
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
        m_EmissionPhase += 0.1f;
        float x = sin(m_EmissionPhase);
        float z = sin(m_EmissionPhase * 0.5f);
        particle.Velocity = glm::vec3(x, 1.0f, z) * glm::length(baseVel);
    }
}
