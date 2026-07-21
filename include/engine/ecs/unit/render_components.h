#pragma once

#include <render/type/graphics_types.h>
#include <render/type/render_data.h>
#include <render/unit/skybox.h>
#include <resource/unit/model.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct MeshRendererComponent
{
    std::shared_ptr<Model> model = nullptr;
    std::string modelName;
    std::weak_ptr<Shader> shader;
    std::string shaderName;
    int order = 0;
    bool castShadow = true;
    bool receiveShadow = true;
    bool ignoreDepth = false;
    RenderMode renderMode = RenderMode::Auto;
    glm::vec4 color = glm::vec4(1.0f);
};

struct PBRMaterialParams
{
    float roughness = 0.5f;
    float metallic = 0.0f;
    float ao = 1.0f;
};

struct MaterialDescriptor
{
    PBRMaterialParams pbr;

    float opacity = 1.0f;
    float alphaCutoff = 0.5f;
    glm::vec3 emission = glm::vec3(0.0f);

    glm::vec2 uvScale = glm::vec2(1.0f);
    glm::vec2 uvOffset = glm::vec2(0.0f);

    std::string albedoPath = "";
    std::string normalPath = "";
    std::string metallicPath = "";
    std::string roughnessPath = "";
    std::string aoPath = "";
    std::string emissivePath = "";
    std::string specularPath = "";

    BlendFactor blendSrc = BlendFactor::SrcAlpha;
    BlendFactor blendDst = BlendFactor::OneMinusSrcAlpha;

    std::string type = "PBR";
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
    uint32_t specularMap = 0;

    uint64_t batchKey = 0;
    bool batchKeyDirty = true;
    bool dirty = true;
};

struct MaterialComponent
{
    MaterialDescriptor desc;
    MaterialGPUState gpu;
};

struct SkyboxRenderComponent
{
    std::shared_ptr<Skybox> skybox = nullptr;
    std::string skyboxName;
    std::weak_ptr<Shader> shader;
    std::string shaderName;
    bool isPrimary = true;

    uint32_t irradianceMap = 0;
    uint32_t prefilterMap = 0;
    uint32_t brdfLUT = 0;
};

struct LODComponent
{
    std::vector<std::shared_ptr<Model>> lodModels;
    std::vector<std::string> lodModelNames;
    std::vector<float> lodDistancesSq;
};

struct OcclusionComponent
{
    uint32_t lastQueryId = 0;
    bool isVisible = true;
    bool queryPending = false;
};

enum class StreamingState
{
    Unloaded,
    Loading,
    Resident,
    Unloading,
    Failed
};

struct StreamingComponent
{
    std::string modelPath;
    bool isStatic = false;
    float loadDistance = 100.0f;
    float unloadDistance = 150.0f;
    bool isRequested = false;
    bool isResident = false;
    StreamingState state = StreamingState::Unloaded;
};
