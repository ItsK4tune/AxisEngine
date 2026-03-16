#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <render/type/graphics_types.h>
#include <render/logic/model.h>
#include <render/logic/shader.h>
#include <render/logic/skybox.h>

// --- Mesh Renderer ---

struct MeshRendererComponent
{
    std::shared_ptr<Model> model = nullptr;
    std::weak_ptr<Shader> shader;
    int order = 0;
    bool castShadow = true;
    bool receiveShadow = true;
    glm::vec4 color = glm::vec4(1.0f);
};

// --- Material ---

enum class MaterialType
{
    PHONG,
    PBR
};

struct MaterialDescriptor
{
    MaterialType type = MaterialType::PHONG;

    float roughness = 0.5f;
    float metallic = 0.0f;
    float ao = 1.0f;
    float opacity = 1.0f;
    float alphaCutoff = 0.5f;
    glm::vec3 emission = glm::vec3(0.0f);

    float shininess = 32.0f;
    glm::vec3 specular = glm::vec3(0.5f);
    glm::vec3 ambient = glm::vec3(1.0f);

    glm::vec2 uvScale = glm::vec2(1.0f);
    glm::vec2 uvOffset = glm::vec2(0.0f);

    std::string albedoPath = "";
    std::string normalPath = "";
    std::string metallicPath = "";
    std::string roughnessPath = "";
    std::string aoPath = "";
    std::string emissivePath = "";

    BlendFactor blendSrc = BlendFactor::SrcAlpha;
    BlendFactor blendDst = BlendFactor::OneMinusSrcAlpha;

    ShaderPorts ports;
};

struct MaterialGPUState
{
    uint32_t albedoMap = 0;
    uint32_t normalMap = 0;
    uint32_t metallicMap = 0;
    uint32_t roughnessMap = 0;
    uint32_t aoMap = 0;
    uint32_t emissiveMap = 0;

    bool dirty = true;
};

struct MaterialComponent
{
    MaterialDescriptor desc;
    MaterialGPUState gpu;
};

// --- Skybox ---

struct SkyboxRenderComponent
{
    std::shared_ptr<Skybox> skybox = nullptr;
    std::weak_ptr<Shader> shader;
    bool isPrimary = true;

    // IBL Maps
    uint32_t irradianceMap = 0;
    uint32_t prefilterMap = 0;
    uint32_t brdfLUT = 0;
};

// --- LOD ---

struct LODComponent
{
    std::vector<std::shared_ptr<Model>> lodModels;
    std::vector<float> lodDistancesSq;
};

// --- Occlusion ---

struct OcclusionComponent {
    uint32_t lastQueryId = 0;
    bool isVisible = true;
    bool queryPending = false;
};

// --- Streaming ---

struct StreamingComponent
{
    std::string modelPath;
    bool isStatic = false;
    float loadDistance = 100.0f;
    bool isRequested = false;
};
