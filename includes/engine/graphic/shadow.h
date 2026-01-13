#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <engine/graphic/shader.h>

class Shadow {
public:
    Shadow();
    ~Shadow();

    void Init(unsigned int width = 2048, unsigned int height = 2048, 
              unsigned int pointWidth = 1024, unsigned int pointHeight = 1024);

    void BindFBO_Dir();
    void BindFBO_Point(int index);
    void UnbindFBO();

    void BindTexture_Dir(int unit);
    void BindTexture_Point(int index, int unit);

    unsigned int GetShadowWidth() const { return SHADOW_WIDTH; }
    unsigned int GetShadowHeight() const { return SHADOW_HEIGHT; }
    unsigned int GetShadowPointWidth() const { return SHADOW_POINT_WIDTH; }
    unsigned int GetShadowPointHeight() const { return SHADOW_POINT_HEIGHT; }

    Shader* GetShaderDir() { return m_ShadowShaderDir; }
    Shader* GetShaderPoint() { return m_ShadowShaderPoint; }

    void SetShaderDir(Shader* shader) { m_ShadowShaderDir = shader; }
    void SetShaderPoint(Shader* shader) { m_ShadowShaderPoint = shader; }

    static const int MAX_POINT_LIGHTS_SHADOW = 4;

private:
    unsigned int SHADOW_WIDTH, SHADOW_HEIGHT;
    unsigned int SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT;

    unsigned int m_ShadowFBO_Dir = 0;
    unsigned int m_ShadowMap_Dir = 0;

    unsigned int m_ShadowFBO_Point[MAX_POINT_LIGHTS_SHADOW];
    unsigned int m_ShadowMap_Point[MAX_POINT_LIGHTS_SHADOW];

    Shader* m_ShadowShaderDir = nullptr;
    Shader* m_ShadowShaderPoint = nullptr;
};
