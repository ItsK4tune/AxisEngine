#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <scene/scene.h>
#include <vector>
#include <memory>
#include <rendering/core/gpu_resources.h>

class Shader;
class IGraphicsContext;
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
    void Init(IGraphicsContext& context);
    void UploadLightData(Scene &scene, Shader *shader);

    int GetDirLightCount() const { return (int)m_DirLights.size(); }
    int GetPointLightCount() const { return (int)m_PointLights.size(); }
    int GetSpotLightCount() const { return (int)m_SpotLights.size(); }

private:
    std::unique_ptr<Graphics::GPUSSBO> m_DirLightSSBO;
    std::unique_ptr<Graphics::GPUSSBO> m_PointLightSSBO;
    std::unique_ptr<Graphics::GPUSSBO> m_SpotLightSSBO;

    std::vector<GPUDirLight> m_DirLights;
    std::vector<GPUPointLight> m_PointLights;
    std::vector<GPUSpotLight> m_SpotLights;

    IGraphicsContext* m_Context = nullptr;
};
