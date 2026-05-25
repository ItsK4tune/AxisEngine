#pragma once

#include <core/unit/aabb.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

enum class RenderMode
{
    Auto,
    ForceForward,
    ForceDeferred
};

class Model;
class Shader;
struct AxisMaterialComponent;
struct ReflectiveComponent;
struct ReflectionProbeComponent;

struct RenderItem
{
    Model* model = nullptr;
    Shader* shader = nullptr;
    AxisMaterialComponent* material = nullptr;
    ReflectiveComponent* reflection = nullptr;
    ReflectionProbeComponent* probe = nullptr;
    glm::vec3 probePos = glm::vec3(0.0f);
    float reflectionIntensity = 1.0f;
    int probeIndex = -1;
    glm::mat4 worldMatrix;
    glm::vec4 tintColor = glm::vec4(1.0f);
    AABB worldAABB;
    float distanceSq = 0.0f;
    uint32_t entityId = 0;
    uint32_t layer = 1;
    int renderOrder = 0;
    bool castShadow = true;
    bool receiveShadow = true;
    bool isTransparent = false;
    bool ignoreDepth = false;
    RenderMode renderMode = RenderMode::Auto;
    uint64_t sortKey = 0;
    uint64_t materialBatchKey = 0;

    bool hasAnimation = false;
    bool isStatic = false;
    const std::vector<glm::mat4>* boneMatrices = nullptr;
};

enum class RenderLightType
{
    Directional,
    Point,
    Spot
};

struct RenderLight
{
    RenderLightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float range;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float innerCutoff;
    float outerCutoff;

    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(0.5f);

    bool castShadows;
    glm::mat4 viewProj;
    int shadowMapIndex = -1;
    uint32_t version = 0;
};

struct RenderSceneData
{
    std::vector<RenderItem> deferredOpaqueItems;
    std::vector<RenderItem> forwardOpaqueItems;
    std::vector<RenderItem> transparentItems;
    std::vector<RenderItem> shadowQueue;
    std::vector<RenderLight> lights;

    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 cameraPosition;
    float nearPlane;
    float farPlane;

    unsigned int irradianceMap = 0;
    unsigned int prefilterMap = 0;
    unsigned int brdfLUT = 0;
};
