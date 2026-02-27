#pragma once

#include <scene/scene.h>
#include <glm/glm.hpp>
#include <vector>

class Shader;
class IBufferManager;

struct GPUDirLight
{
    glm::vec3 direction;
    float shadowIndex;
    glm::vec3 color;
    float intensity;
    glm::vec3 ambient;
    float pad1;
    glm::vec3 diffuse;
    float pad2;
    glm::vec3 specular;
    float pad3;
};

struct GPUPointLight
{
    glm::vec3 position;
    float shadowIndex;
    glm::vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
    glm::vec3 ambient;
    float pad1;
    glm::vec3 diffuse;
    float pad2;
    glm::vec3 specular;
    float pad3;
};

struct GPUSpotLight
{
    glm::vec3 position;
    float pad0;
    glm::vec3 direction;
    float shadowIndex;
    glm::vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    float pad2;
    float pad3;
    float pad4;
    glm::vec3 ambient;
    float pad5;
    glm::vec3 diffuse;
    float pad6;
    glm::vec3 specular;
    float pad7;
};

class LightRenderer
{
public:
    void Init(IBufferManager& bufferManager);
    void UploadLightData(Scene &scene, Shader *shader);

    int GetDirLightCount() const { return (int)m_DirLights.size(); }
    int GetPointLightCount() const { return (int)m_PointLights.size(); }
    int GetSpotLightCount() const { return (int)m_SpotLights.size(); }

private:
    unsigned int m_DirLightSSBO = 0;
    unsigned int m_PointLightSSBO = 0;
    unsigned int m_SpotLightSSBO = 0;

    std::vector<GPUDirLight> m_DirLights;
    std::vector<GPUPointLight> m_PointLights;
    std::vector<GPUSpotLight> m_SpotLights;

    IBufferManager* m_BufferManager = nullptr;
};
