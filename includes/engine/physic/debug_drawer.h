#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <LinearMath/btIDebugDraw.h>

class DebugDrawer : public btIDebugDraw
{
public:
    DebugDrawer();
    virtual ~DebugDrawer();

    void Init();
    void FrameStart();
    void Flush();

    virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;
    virtual void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override;
    virtual void reportErrorWarning(const char *warningString) override;
    virtual void draw3dText(const btVector3 &location, const char *textString) override;
    virtual void setDebugMode(int debugMode) override;
    virtual int getDebugMode() const override;

private:
    struct LineVertex
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    std::vector<LineVertex> m_Lines;
    GLuint m_VAO, m_VBO;
    int m_DebugMode;
};
