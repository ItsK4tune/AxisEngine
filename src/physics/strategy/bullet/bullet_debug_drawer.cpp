#include <physics/strategy/bullet/bullet_debug_drawer.h>
#include <core/logic/logger.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>

void BulletDebugDrawer::SetManagers(IBufferManager* bufferManager, IDrawContext* drawContext)
{
    m_BufferManager = bufferManager;
    m_DrawContext = drawContext;
}

IBufferManager& BulletDebugDrawer::GetBufferManager()
{
    if (!m_BufferManager)
    {
        LOGGER_ERROR("BulletDebugDrawer") << "BufferManager not set!";
        throw std::runtime_error("BufferManager not set in BulletDebugDrawer");
    }
    return *m_BufferManager;
}

IDrawContext& BulletDebugDrawer::GetDrawContext()
{
    if (!m_DrawContext)
    {
        LOGGER_ERROR("BulletDebugDrawer") << "DrawContext not set!";
        throw std::runtime_error("DrawContext not set in BulletDebugDrawer");
    }
    return *m_DrawContext;
}

BulletDebugDrawer::BulletDebugDrawer()
    : m_DebugMode(DBG_DrawWireframe | DBG_DrawAabb | DBG_DrawContactPoints), m_VAO(0), m_VBO(0)
{
}

BulletDebugDrawer::~BulletDebugDrawer()
{
    if (m_BufferManager)
    {
        if (m_VAO)
            m_BufferManager->DeleteVertexArrays(1, &m_VAO);
        if (m_VBO)
            m_BufferManager->DeleteBuffers(1, &m_VBO);
    }
}

void BulletDebugDrawer::Initialize()
{
    if (!m_BufferManager || m_VAO != 0)
        return;
    auto& bm = GetBufferManager();

    m_VAO = bm.GenVertexArray();
    m_VBO = bm.GenBuffer();

    bm.BindVertexArray(m_VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_VBO);

    bm.BufferData(BufferType::ArrayBuffer, 0, nullptr, BufferUsage::DynamicDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, sizeof(LineVertex), (void*)0);

    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, sizeof(LineVertex), (void*)offsetof(LineVertex, color));

    bm.BindVertexArray(0);
}

void BulletDebugDrawer::FrameStart()
{
    if (m_VAO == 0 && m_BufferManager)
    {
        Initialize();
    }
}

void BulletDebugDrawer::Flush()
{
    if (m_Lines.empty())
        return;
    if (!m_BufferManager || !m_DrawContext)
    {
        m_Lines.clear();
        return;
    }

    auto& bm = GetBufferManager();
    auto& dc = GetDrawContext();

    bm.BindVertexArray(m_VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, m_VBO);

    bm.BufferData(BufferType::ArrayBuffer, m_Lines.size() * sizeof(LineVertex), m_Lines.data(),
                  BufferUsage::DynamicDraw);

    dc.DrawArrays(Primitive::Lines, 0, (unsigned int)m_Lines.size());

    bm.BindVertexArray(0);
    m_Lines.clear();
}

void BulletDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
    if (!m_BufferManager || !m_DrawContext)
        return;
    m_Lines.push_back({BulletGLMHelpers::convert(from), BulletGLMHelpers::convert(color)});
    m_Lines.push_back({BulletGLMHelpers::convert(to), BulletGLMHelpers::convert(color)});
}

void BulletDebugDrawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance,
                                         int lifeTime, const btVector3& color)
{
    btVector3 to = PointOnB + normalOnB * 1.0f;
    drawLine(PointOnB, to, color);
}

void BulletDebugDrawer::reportErrorWarning(const char* warningString)
{
    LOGGER_WARN("Physics") << warningString;
}

void BulletDebugDrawer::draw3dText(const btVector3&, const char*)
{
    // Bullet requires this callback, but AxisEngine's line renderer has no text primitive.
}

void BulletDebugDrawer::setDebugMode(int debugMode)
{
    m_DebugMode = debugMode;
}

int BulletDebugDrawer::getDebugMode() const
{
    return m_DebugMode;
}
