#include <physic/backends/bullet_debug_drawer.h>
#include <utils/logger.h>
#include <utils/bullet_glm_helpers.h>
#include <interface/graphic/i_buffer_manager.h>
#include <interface/graphic/i_draw_context.h>

IBufferManager* BulletDebugDrawer::s_BufferManager = nullptr;
IDrawContext* BulletDebugDrawer::s_DrawContext = nullptr;

void BulletDebugDrawer::SetManagers(IBufferManager& bufferManager, IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_DrawContext = &drawContext;
}

IBufferManager& BulletDebugDrawer::GetBufferManager()
{
    if (!s_BufferManager) {
        LOGGER_ERROR("BulletDebugDrawer") << "BufferManager not set!";
        throw std::runtime_error("BufferManager not set in BulletDebugDrawer");
    }
    return *s_BufferManager;
}

IDrawContext& BulletDebugDrawer::GetDrawContext()
{
    if (!s_DrawContext) {
        LOGGER_ERROR("BulletDebugDrawer") << "DrawContext not set!";
        throw std::runtime_error("DrawContext not set in BulletDebugDrawer");
    }
    return *s_DrawContext;
}

BulletDebugDrawer::BulletDebugDrawer()
    : m_DebugMode(DBG_DrawWireframe | DBG_DrawAabb | DBG_DrawContactPoints), m_VAO(0), m_VBO(0)
{
}

BulletDebugDrawer::~BulletDebugDrawer()
{
    if (s_BufferManager)
    {
        if (m_VAO) s_BufferManager->DeleteVertexArrays(1, &m_VAO);
        if (m_VBO) s_BufferManager->DeleteBuffers(1, &m_VBO);
    }
}

void BulletDebugDrawer::Init()
{
    if (!s_BufferManager) return;
    auto& bm = GetBufferManager();

    m_VAO = bm.GenVertexArray();
    m_VBO = bm.GenBuffer();

    bm.BindVertexArray(m_VAO);
    bm.BindBuffer(Graphics::BufferType::ArrayBuffer, m_VBO);

    bm.BufferData(Graphics::BufferType::ArrayBuffer, 0, nullptr, Graphics::BufferUsage::DynamicDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, Graphics::DataType::Float, false, sizeof(LineVertex), (void *)0);

    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, Graphics::DataType::Float, false, sizeof(LineVertex), (void *)offsetof(LineVertex, color));

    bm.BindVertexArray(0);
}

void BulletDebugDrawer::FrameStart()
{
    m_Lines.clear();
}

void BulletDebugDrawer::Flush()
{
    if (m_Lines.empty() || !s_BufferManager || !s_DrawContext)
        return;

    auto& bm = GetBufferManager();
    auto& dc = GetDrawContext();

    bm.BindVertexArray(m_VAO);
    bm.BindBuffer(Graphics::BufferType::ArrayBuffer, m_VBO);

    bm.BufferData(Graphics::BufferType::ArrayBuffer, m_Lines.size() * sizeof(LineVertex), m_Lines.data(), Graphics::BufferUsage::DynamicDraw);

    dc.DrawArrays(Graphics::Primitive::Lines, 0, (unsigned int)m_Lines.size());

    bm.BindVertexArray(0);
}

void BulletDebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
{
    m_Lines.push_back({BulletGLMHelpers::convert(from), BulletGLMHelpers::convert(color)});
    m_Lines.push_back({BulletGLMHelpers::convert(to), BulletGLMHelpers::convert(color)});
}

void BulletDebugDrawer::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)
{
    btVector3 to = PointOnB + normalOnB * 1.0f;
    drawLine(PointOnB, to, color);
}

void BulletDebugDrawer::reportErrorWarning(const char *warningString)
{
    LOGGER_WARN("Physics") << warningString;
}

void BulletDebugDrawer::draw3dText(const btVector3 &location, const char *textString)
{
}

void BulletDebugDrawer::setDebugMode(int debugMode)
{
    m_DebugMode = debugMode;
}

int BulletDebugDrawer::getDebugMode() const
{
    return m_DebugMode;
}
