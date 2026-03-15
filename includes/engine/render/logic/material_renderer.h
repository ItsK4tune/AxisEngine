#pragma once

#include <glm/glm.hpp>
#include <scene/logic/scene.h>

class IGraphicsContext;
class Shader;

class MaterialRenderer {
public:
    void Initialize(IGraphicsContext& context, unsigned int whiteTextureId, unsigned int blackTextureId = 0, unsigned int flatNormalTextureId = 0);
    bool SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene, bool debugNoTexture);

private:
    IGraphicsContext* m_Context = nullptr;
    unsigned int m_WhiteTextureID = 0;
    unsigned int m_BlackTextureID = 0;
    unsigned int m_FlatNormalTextureID = 0;
};