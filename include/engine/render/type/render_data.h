#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

#include <core/unit/aabb.h>

// Forward declarations to avoid direct component dependency in header
class Model;
class Shader;
struct AxisMaterialComponent;
struct ReflectiveComponent;
struct ReflectionProbeComponent;

struct RenderItem {
    Model* model = nullptr;
    Shader* shader = nullptr;
    AxisMaterialComponent* material = nullptr;
    ReflectiveComponent* reflection = nullptr;
    ReflectionProbeComponent* probe = nullptr;
    glm::vec3 probePos = glm::vec3(0.0f);
    float reflectionIntensity = 1.0f;
    int probeIndex = -1; // New field for Deferred Reflection Binding
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
    uint64_t sortKey = 0;

    // Animation data extracted from ECS during queue build
    bool hasAnimation = false;
    bool isStatic = false;
    std::vector<glm::mat4> boneMatrices;
};

enum class RenderLightType {
    Directional,
    Point,
    Spot
};

struct RenderLight {
    RenderLightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float range;           // For Point/Spot
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float innerCutoff;     // For Spot (cos)
    float outerCutoff;     // For Spot (cos)
    
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(0.5f);

    bool castShadows;
    glm::mat4 viewProj;
    int shadowMapIndex = -1;
    uint32_t version = 0;
};

struct RenderSceneData {
    std::vector<RenderItem> opaqueItems;
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
