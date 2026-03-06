#pragma once
#include <scene/scene.h>
#include <glm/glm.hpp>

class Shader;
class IGraphicsContext;

class MaterialRenderer {
public:
    void Init(IGraphicsContext& context, unsigned int whiteTextureId);
    void SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene, bool debugNoTexture);

private:
    IGraphicsContext* m_Context = nullptr;
    unsigned int m_WhiteTextureID = 0;
};
