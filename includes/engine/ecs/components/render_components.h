#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <graphic/geometry/model.h>
#include <graphic/renderer/ui_model.h>
#include <graphic/renderer/skybox.h>
#include <graphic/core/shader.h>
#include <interface/graphic/graphics_types.h>

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#ifdef constant
#undef constant
#endif

struct MeshRendererComponent
{
    std::shared_ptr<Model> model = nullptr;
    std::weak_ptr<Shader> shader;
    int order = 0;
    bool castShadow = true;
    glm::vec4 color = glm::vec4(1.0f);
};

enum class MaterialType
{
    PHONG,
    PBR
};

struct MaterialComponent
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

    // Texture slots (Path for loading, ID for rendering)
    std::string albedoPath = "";
    std::string normalPath = "";
    std::string metallicPath = "";
    std::string roughnessPath = "";
    std::string aoPath = "";
    std::string emissivePath = "";

    uint32_t albedoMap = 0;
    uint32_t normalMap = 0;
    uint32_t metallicMap = 0;
    uint32_t roughnessMap = 0;
    uint32_t aoMap = 0;
    uint32_t emissiveMap = 0;

    // Blending state
    Graphics::BlendFactor blendSrc = Graphics::BlendFactor::SrcAlpha;
    Graphics::BlendFactor blendDst = Graphics::BlendFactor::OneMinusSrcAlpha;
};

struct SkyboxRenderComponent
{
    std::shared_ptr<Skybox> skybox = nullptr;
    std::weak_ptr<Shader> shader;
};

struct LODComponent
{
    std::vector<std::shared_ptr<Model>> lodModels;
    std::vector<float> lodDistancesSq;
};
