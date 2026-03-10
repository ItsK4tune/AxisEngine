#pragma once

#include <glm/glm.hpp>
#include <scene/logic/scene.h>

class IGraphicsContext;
class Shader;

class MaterialRenderer {
public:
    void Initialize(IGraphicsContext& context, unsigned int whiteTextureId);
    void SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene, bool debugNoTexture);

private:
    IGraphicsContext* m_Context = nullptr;
    unsigned int m_WhiteTextureID = 0;
};