#include <graphic/renderer/shadow.h>
#include <iostream>

Shadow::Shadow()
{
    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Dir[i] = 0;
        m_ShadowMap_Dir[i] = 0;
    }
    
    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Point[i] = 0;
        m_ShadowMap_Point[i] = 0;
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        m_ShadowFBO_Spot[i] = 0;
        m_ShadowMap_Spot[i] = 0;
    }
}

Shadow::~Shadow()
{
    Shutdown();
}

void Shadow::Shutdown()
{
    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        if (m_ShadowFBO_Dir[i] != 0)
        {
            glDeleteFramebuffers(1, &m_ShadowFBO_Dir[i]);
            m_ShadowFBO_Dir[i] = 0;
        }
        if (m_ShadowMap_Dir[i] != 0)
        {
            glDeleteTextures(1, &m_ShadowMap_Dir[i]);
            m_ShadowMap_Dir[i] = 0;
        }
    }

    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        if (m_ShadowFBO_Point[i] != 0)
        {
            glDeleteFramebuffers(1, &m_ShadowFBO_Point[i]);
            m_ShadowFBO_Point[i] = 0;
        }
        if (m_ShadowMap_Point[i] != 0)
        {
            glDeleteTextures(1, &m_ShadowMap_Point[i]);
            m_ShadowMap_Point[i] = 0;
        }
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        if (m_ShadowFBO_Spot[i] != 0)
        {
            glDeleteFramebuffers(1, &m_ShadowFBO_Spot[i]);
            m_ShadowFBO_Spot[i] = 0;
        }
        if (m_ShadowMap_Spot[i] != 0)
        {
            glDeleteTextures(1, &m_ShadowMap_Spot[i]);
            m_ShadowMap_Spot[i] = 0;
        }
    }
}

void Shadow::Init(unsigned int width, unsigned int height, unsigned int pointWidth, unsigned int pointHeight)
{
    SHADOW_WIDTH = width;
    SHADOW_HEIGHT = height;
    SHADOW_POINT_WIDTH = pointWidth;
    SHADOW_POINT_HEIGHT = pointHeight;

    for (int i = 0; i < MAX_DIR_LIGHTS_SHADOW; ++i)
    {
        glGenFramebuffers(1, &m_ShadowFBO_Dir[i]);
        glGenTextures(1, &m_ShadowMap_Dir[i]);
        glBindTexture(GL_TEXTURE_2D, m_ShadowMap_Dir[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO_Dir[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowMap_Dir[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    for (int i = 0; i < MAX_POINT_LIGHTS_SHADOW; ++i)
    {
        glGenFramebuffers(1, &m_ShadowFBO_Point[i]);
        glGenTextures(1, &m_ShadowMap_Point[i]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_ShadowMap_Point[i]);
        for (unsigned int j = 0; j < 6; ++j)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_DEPTH_COMPONENT,
                         SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO_Point[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_ShadowMap_Point[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    for (int i = 0; i < MAX_SPOT_LIGHTS_SHADOW; ++i)
    {
        glGenFramebuffers(1, &m_ShadowFBO_Spot[i]);
        glGenTextures(1, &m_ShadowMap_Spot[i]);
        glBindTexture(GL_TEXTURE_2D, m_ShadowMap_Spot[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO_Spot[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_ShadowMap_Spot[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Shadow::BindFBO_Dir(int index)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO_Dir[index]);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::BindFBO_Point(int index)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO_Point[index]);
        glViewport(0, 0, SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT);
    }
}

void Shadow::BindFBO_Spot(int index)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_ShadowFBO_Spot[index]);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    }
}

void Shadow::UnbindFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Shadow::BindTexture_Dir(int index, int unit)
{
    if (index >= 0 && index < MAX_DIR_LIGHTS_SHADOW)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_ShadowMap_Dir[index]);
    }
}

void Shadow::BindTexture_Point(int index, int unit)
{
    if (index >= 0 && index < MAX_POINT_LIGHTS_SHADOW)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_ShadowMap_Point[index]);
    }
}

void Shadow::BindTexture_Spot(int index, int unit)
{
    if (index >= 0 && index < MAX_SPOT_LIGHTS_SHADOW)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_ShadowMap_Spot[index]);
    }
}
