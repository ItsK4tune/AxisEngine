#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <graphic/core/shader.h>

class Shadow {
public:
    Shadow();
    ~Shadow();

    void Init(unsigned int width = 2048, unsigned int height = 2048, 
              unsigned int pointWidth = 1024, unsigned int pointHeight = 1024);
    void Shutdown();

    void BindFBO_Dir(int index);
    void BindFBO_Point(int index);
    void BindFBO_Spot(int index);
    void UnbindFBO();

    void BindTexture_Dir(int index, int unit);
    void BindTexture_Point(int index, int unit);
    void BindTexture_Spot(int index, int unit);

    unsigned int GetShadowWidth() const { return SHADOW_WIDTH; }
    unsigned int GetShadowHeight() const { return SHADOW_HEIGHT; }
    unsigned int GetShadowPointWidth() const { return SHADOW_POINT_WIDTH; }
    unsigned int GetShadowPointHeight() const { return SHADOW_POINT_HEIGHT; }

    Shader* GetShaderDir() { return m_ShadowShaderDir; }
    Shader* GetShaderPoint() { return m_ShadowShaderPoint; }
    Shader* GetShaderSpot() { return m_ShadowShaderSpot; }

    void SetShaderDir(Shader* shader) { m_ShadowShaderDir = shader; }
    void SetShaderPoint(Shader* shader) { m_ShadowShaderPoint = shader; }
    void SetShaderSpot(Shader* shader) { m_ShadowShaderSpot = shader; }

    static const int MAX_DIR_LIGHTS_SHADOW = 2;
    static const int MAX_POINT_LIGHTS_SHADOW = 2;
    static const int MAX_SPOT_LIGHTS_SHADOW = 2;

private:
    unsigned int SHADOW_WIDTH, SHADOW_HEIGHT;
    unsigned int SHADOW_POINT_WIDTH, SHADOW_POINT_HEIGHT;

    unsigned int m_ShadowFBO_Dir[MAX_DIR_LIGHTS_SHADOW];
    unsigned int m_ShadowMap_Dir[MAX_DIR_LIGHTS_SHADOW];

    unsigned int m_ShadowFBO_Point[MAX_POINT_LIGHTS_SHADOW];
    unsigned int m_ShadowMap_Point[MAX_POINT_LIGHTS_SHADOW];

    unsigned int m_ShadowFBO_Spot[MAX_SPOT_LIGHTS_SHADOW];
    unsigned int m_ShadowMap_Spot[MAX_SPOT_LIGHTS_SHADOW];

    Shader* m_ShadowShaderDir = nullptr;
    Shader* m_ShadowShaderPoint = nullptr;
    Shader* m_ShadowShaderSpot = nullptr;
};
