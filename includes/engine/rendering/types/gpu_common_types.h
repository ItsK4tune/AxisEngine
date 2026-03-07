#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <rendering/renderer/shadow.h>

namespace Graphics {

struct GPUCameraData {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec3 viewPos;
    float pad0;
};

struct GPUGlobalLightData {
    glm::mat4 lightSpaceMatricesDir[Shadow::MAX_DIR_LIGHTS_SHADOW];
    glm::mat4 lightSpaceMatricesSpot[Shadow::MAX_SPOT_LIGHTS_SHADOW];
    int numDirLights;
    int nrPointLights;
    int nrSpotLights;
    int u_ReceiveShadow;
    float farPlanePoint;
    float farPlaneSpot;
    float pad0;
    float pad1;
};

}
