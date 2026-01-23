#include <graphic/core/post_process_pipeline.h>
#include <ecs/systems/render_system.h>
#include <resource/resource_manager.h>
#include <iostream>

PostProcessPipeline::PostProcessPipeline() {}

PostProcessPipeline::~PostProcessPipeline()
{
    Shutdown();
}

void PostProcessPipeline::Init(int width, int height, ResourceManager &res)
{
    m_Width = width;
    m_Height = height;
    InitQuad();
    InitFramebuffers();
    
    // Load AA shaders
    res.LoadShader("fxaa", "src/asset/shaders/fxaa.vs", "src/asset/shaders/fxaa.fs");
    res.LoadShader("taa", "src/asset/shaders/taa.vs", "src/asset/shaders/taa.fs");
    
    m_FXAAShader = res.GetShader("fxaa");
    m_TAAShader = res.GetShader("taa");
}

void PostProcessPipeline::InitFramebuffers()
{
    glGenFramebuffers(2, m_FBO);
    glGenTextures(2, m_ColorBuffers);
    glGenTextures(1, &m_DepthTexture);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[i]);
        glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorBuffers[i], 0);
        
        if (i == 0)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "[PostProcessPipeline] FBO " << i << " is not complete!" << std::endl;
    }
    
    // TAA History buffer
    glGenFramebuffers(1, &m_HistoryFBO);
    glGenTextures(1, &m_HistoryTexture);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_HistoryFBO);
    glBindTexture(GL_TEXTURE_2D, m_HistoryTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_HistoryTexture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "[PostProcessPipeline] History FBO is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessPipeline::Shutdown()
{
    if (m_FBO[0] != 0) { glDeleteFramebuffers(2, m_FBO); m_FBO[0] = 0; m_FBO[1] = 0; }
    if (m_ColorBuffers[0] != 0) { glDeleteTextures(2, m_ColorBuffers); m_ColorBuffers[0] = 0; m_ColorBuffers[1] = 0; }
    if (m_DepthTexture != 0) { glDeleteTextures(1, &m_DepthTexture); m_DepthTexture = 0; }
    if (m_HistoryFBO != 0) { glDeleteFramebuffers(1, &m_HistoryFBO); m_HistoryFBO = 0; }
    if (m_HistoryTexture != 0) { glDeleteTextures(1, &m_HistoryTexture); m_HistoryTexture = 0; }
    if (m_QuadVAO != 0) { glDeleteVertexArrays(1, &m_QuadVAO); m_QuadVAO = 0; }
    if (m_QuadVBO != 0) { glDeleteBuffers(1, &m_QuadVBO); m_QuadVBO = 0; }
}

void PostProcessPipeline::Resize(int width, int height)
{
    m_Width = width;
    m_Height = height;

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    }

    glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glBindTexture(GL_TEXTURE_2D, m_HistoryTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
}

void PostProcessPipeline::BeginCapture()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[0]);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessPipeline::EndCapture()
{
    glDisable(GL_DEPTH_TEST);

    int readIdx = 0;
    int writeIdx = 1;

    for (const auto &effect : m_Effects)
    {
        if (!effect.shader)
            continue;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO[readIdx]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO[writeIdx]);
        glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[writeIdx]);

        effect.shader->use();
        effect.shader->setInt("screenTexture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[readIdx]);

        glBindVertexArray(m_QuadVAO);

        if (effect.width > 0 && effect.height > 0)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(effect.x, m_Height - effect.y - effect.height, effect.width, effect.height);
        }

        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (effect.width > 0 && effect.height > 0)
        {
            glDisable(GL_SCISSOR_TEST);
        }

        readIdx = writeIdx;
        writeIdx = 1 - readIdx;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO[readIdx]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void PostProcessPipeline::ApplyAntiAliasing(AntiAliasingMode mode, const glm::mat4 &prevViewProj, const glm::mat4 &currViewProj, const glm::vec2 &jitterOffset)
{
    if (mode == AntiAliasingMode::NONE)
        return;
    
    Shader* shader = nullptr;
    if (mode == AntiAliasingMode::FXAA)
        shader = m_FXAAShader;
    else if (mode == AntiAliasingMode::TAA)
        shader = m_TAAShader;
        
    if (!shader) 
    {
        std::cerr << "[PostProcess] [ERROR] AA Shader not found for mode " << (int)mode << "!" << std::endl;
        return;
    }

    glDisable(GL_DEPTH_TEST);
    
    shader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[0]);
    shader->setInt("screenTexture", 0);
    
    if (mode == AntiAliasingMode::TAA)
    {
        shader->setInt("depthTexture", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
        
        shader->setInt("historyTexture", 2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_HistoryTexture);
        
        shader->setMat4("invViewProj", glm::inverse(currViewProj));
        shader->setMat4("prevViewProj", prevViewProj);
        shader->setVec2("jitterOffset", jitterOffset);
        
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[1]);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO[1]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_HistoryFBO);
        glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO[1]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO[0]);
        glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    else if (mode == AntiAliasingMode::FXAA)
    {
        shader->setVec2("inverseScreenSize", glm::vec2(1.0f / m_Width, 1.0f / m_Height));
        
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[1]);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO[1]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO[0]);
        glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessPipeline::AddEffect(Shader *shader)
{
    if (shader)
    {
        m_Effects.push_back({shader, 0, 0, 0, 0});
    }
}

void PostProcessPipeline::AddEffect(Shader *shader, int x, int y, int w, int h)
{
    if (shader)
    {
        m_Effects.push_back({shader, x, y, w, h});
    }
}

void PostProcessPipeline::ClearEffects()
{
    m_Effects.clear();
}

void PostProcessPipeline::InitQuad()
{
    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f};
    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);
    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
}