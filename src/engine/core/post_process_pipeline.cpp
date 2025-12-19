#include <engine/core/post_process_pipeline.h>
#include <iostream>

PostProcessPipeline::PostProcessPipeline() {}

PostProcessPipeline::~PostProcessPipeline()
{
    glDeleteFramebuffers(2, m_FBO);
    glDeleteTextures(2, m_ColorBuffers);
    glDeleteRenderbuffers(1, &m_RBO);
    glDeleteVertexArrays(1, &m_QuadVAO);
    glDeleteBuffers(1, &m_QuadVBO);
}

void PostProcessPipeline::Init(int width, int height)
{
    m_Width = width;
    m_Height = height;
    InitQuad();
    InitFramebuffers();
}

void PostProcessPipeline::InitFramebuffers()
{
    glGenFramebuffers(2, m_FBO);
    glGenTextures(2, m_ColorBuffers);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO[i]);
        glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorBuffers[i], 0);

        if (i == 0)
        {
            glGenRenderbuffers(1, &m_RBO);
            glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "[PostProcessPipeline] FBO " << i << " is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessPipeline::Resize(int width, int height)
{
    m_Width = width;
    m_Height = height;

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
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