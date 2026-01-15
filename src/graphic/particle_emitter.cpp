#include <graphic/particle_emitter.h>
#include <glad/glad.h>
#include <cstdlib>
#include <ctime>

ParticleEmitter::ParticleEmitter() : m_VAO(0), m_VBO(0) {
    // Seed random (better to do this centrally in Application but okay here for now)
    // std::srand(static_cast<unsigned int>(std::time(0))); 
}

ParticleEmitter::~ParticleEmitter() {
    if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO != 0) glDeleteBuffers(1, &m_VBO);
}

ParticleEmitter::ParticleEmitter(ParticleEmitter&& other) noexcept
    : m_Particles(std::move(other.m_Particles)),
      m_MaxParticles(other.m_MaxParticles),
      m_LastUsedParticle(other.m_LastUsedParticle),
      m_VAO(other.m_VAO),
      m_VBO(other.m_VBO),
      texture(other.texture),
      // Copy other config props
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
}

ParticleEmitter& ParticleEmitter::operator=(ParticleEmitter&& other) noexcept {
    if (this != &other) {
        if (m_VAO != 0) glDeleteVertexArrays(1, &m_VAO);
        if (m_VBO != 0) glDeleteBuffers(1, &m_VBO);

        m_Particles = std::move(other.m_Particles);
        m_MaxParticles = other.m_MaxParticles;
        m_LastUsedParticle = other.m_LastUsedParticle;
        m_VAO = other.m_VAO;
        m_VBO = other.m_VBO;
        texture = other.texture;

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
    }
    return *this;
}

void ParticleEmitter::Init(unsigned int maxParticles) {
    m_MaxParticles = maxParticles;
    m_Particles.resize(m_MaxParticles);

    float quadVertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
         -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    
    glBindVertexArray(0);
}

// Internal helper for random float 0..1
float RandomFloat() {
    return (float)rand() / (float)RAND_MAX;
}

float RandomFloat(float min, float max) {
    return min + RandomFloat() * (max - min);
}

void ParticleEmitter::Update(float dt, const glm::vec3& offset) {
    m_SpawnAccumulator += dt;
    float rate = (SpawnRate > 0.0f) ? (1.0f / SpawnRate) : 0.0f;
    
    while (m_SpawnAccumulator >= rate && rate > 0.0001f) {
        int unusedParticle = FirstUnusedParticle();
        RespawnParticle(m_Particles[unusedParticle], offset);
        m_SpawnAccumulator -= rate;
    }

    for (unsigned int i = 0; i < m_MaxParticles; ++i) {
        Particle &p = m_Particles[i];
        p.Life -= dt;
        if (p.Life > 0.0f) {
            p.Position += p.Velocity * dt;
            
            float t = 1.0f - (p.Life / p.StartLife);
            p.Color = StartColor * (1.0f - t) + EndColor * t;
            p.Size = StartSize * (1.0f - t) + EndSize * t;
        }
    }
}

void ParticleEmitter::Render(Shader* shader) {
    if (texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->id);
        shader->setInt("sprite", 0);
    }
    
    glBindVertexArray(m_VAO);
    
    for (Particle &particle : m_Particles) {
        if (particle.Life > 0.0f) {
            shader->setVec3("offset", particle.Position);
            shader->setVec4("color", particle.Color);
            shader->setFloat("scale", particle.Size);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
    
    glBindVertexArray(0);
}

unsigned int ParticleEmitter::FirstUnusedParticle() {
    for (unsigned int i = m_LastUsedParticle; i < m_MaxParticles; ++i) {
        if (m_Particles[i].Life <= 0.0f) {
            m_LastUsedParticle = i;
            return i;
        }
    }
    for (unsigned int i = 0; i < m_LastUsedParticle; ++i) {
        if (m_Particles[i].Life <= 0.0f) {
            m_LastUsedParticle = i;
            return i;
        }
    }
    return 0;
}

void ParticleEmitter::RespawnParticle(Particle& particle, const glm::vec3& offset) {
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
    
    if (Shape == EmissionShape::DIRECTIONAL) {
        particle.Velocity = baseVel;
    }
    else if (Shape == EmissionShape::CONE) {
        float angleX = RandomFloat(-0.5f, 0.5f); 
        float angleZ = RandomFloat(-0.5f, 0.5f);
        glm::vec3 dir = glm::normalize(glm::vec3(angleX, 1.0f, angleZ));
        float speed = glm::length(baseVel);
        particle.Velocity = dir * speed;
    }
    else if (Shape == EmissionShape::FIGURE_EIGHT) {
        static float timeAcc = 0.0f;
        timeAcc += 0.1f;
        float x = sin(timeAcc);
        float z = sin(timeAcc / 2.0f);
        particle.Velocity = glm::vec3(x, 1.0f, z) * glm::length(baseVel);
    }
}
