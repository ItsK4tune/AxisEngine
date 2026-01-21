#include <physic/debug_drawer.h>
#include <iostream>
#include <utils/bullet_glm_helpers.h>

DebugDrawer::DebugDrawer()
    : m_DebugMode(DBG_DrawWireframe | DBG_DrawAabb | DBG_DrawContactPoints), m_VAO(0), m_VBO(0)
{
}

DebugDrawer::~DebugDrawer()
{
    if (m_VAO)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
    if (m_VBO)
    {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }
}

void DebugDrawer::Init()
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void *)offsetof(LineVertex, color));

    glBindVertexArray(0);
}

void DebugDrawer::FrameStart()
{
    m_Lines.clear();
}

void DebugDrawer::Flush()
{
    if (m_Lines.empty())
        return;

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferData(GL_ARRAY_BUFFER, m_Lines.size() * sizeof(LineVertex), m_Lines.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_LINES, 0, (GLsizei)m_Lines.size());

    glBindVertexArray(0);
}

void DebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
{
    m_Lines.push_back({BulletGLMHelpers::convert(from), BulletGLMHelpers::convert(color)});
    m_Lines.push_back({BulletGLMHelpers::convert(to), BulletGLMHelpers::convert(color)});
}

void DebugDrawer::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)
{
    btVector3 to = PointOnB + normalOnB * 1.0f;
    drawLine(PointOnB, to, color);
}

void DebugDrawer::reportErrorWarning(const char *warningString)
{
    std::cerr << "[Physics Warning] " << warningString << std::endl;
}

void DebugDrawer::draw3dText(const btVector3 &location, const char *textString)
{
    // TODO: Implement text rendering if needed
}

void DebugDrawer::setDebugMode(int debugMode)
{
    m_DebugMode = debugMode;
}

int DebugDrawer::getDebugMode() const
{
    return m_DebugMode;
}
