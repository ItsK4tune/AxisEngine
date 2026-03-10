#include <render/logic/material_renderer.h>
#include <ecs/unit/render_components.h>
#include <render/logic/shader.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>

void MaterialRenderer::Initialize(IGraphicsContext& context, unsigned int whiteTextureId) {
    m_Context = &context;
    m_WhiteTextureID = whiteTextureId;
}

void MaterialRenderer::SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene, bool debugNoTexture) {
    if (scene.registry.all_of<MaterialComponent>(entity)) {
        auto &mat = scene.registry.get<MaterialComponent>(entity);
        if (mat.desc.type == MaterialType::PBR) {
            shader->setFloat("material.roughness", mat.desc.roughness);
            shader->setFloat("material.metallic", mat.desc.metallic);
            shader->setFloat("material.ao", mat.desc.ao);
            shader->setVec3("material.emission", mat.desc.emission);
        } else {
            shader->setFloat("material.shininess", mat.desc.shininess);
            shader->setVec3("material.specular", mat.desc.specular);
            shader->setVec3("material.ambient", mat.desc.ambient);
            shader->setVec3("material.emission", mat.desc.emission);
        }
        shader->setFloat("material.opacity", mat.desc.opacity);
        shader->setVec2("uvScale", mat.desc.uvScale);
        shader->setVec2("uvOffset", mat.desc.uvOffset);
        shader->setCustomPorts(mat.desc.ports);

        if (debugNoTexture) {
            m_Context->GetTextureManager().ActiveTexture(TextureUnit::Texture0);
            m_Context->GetTextureManager().BindTexture(TextureType::Texture2D, m_WhiteTextureID);
        } else {
            auto& tm = m_Context->GetTextureManager();
            if (mat.gpu.albedoMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture0);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.albedoMap);
                shader->setInt("texture_diffuse1", 0);
            }
            if (mat.gpu.normalMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture1);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.normalMap);
                shader->setInt("texture_normal1", 1);
            }
            if (mat.gpu.metallicMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture2);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.metallicMap);
                shader->setInt("texture_metallic1", 2);
            }
            if (mat.gpu.roughnessMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture3);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.roughnessMap);
                shader->setInt("texture_roughness1", 3);
            }
            if (mat.gpu.aoMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture4);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.aoMap);
                shader->setInt("texture_ao1", 4);
            }
            if (mat.gpu.emissiveMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture5);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.emissiveMap);
                shader->setInt("texture_emissive1", 5);
            }
        }
    } else {
        shader->setFloat("material.shininess", 32.0f);
        shader->setVec3("material.specular", glm::vec3(0.5f));
        shader->setVec3("material.ambient", glm::vec3(1.0f));
        shader->setVec3("material.emission", glm::vec3(0.0f));
        shader->setFloat("material.opacity", 1.0f);
        shader->setVec2("uvScale", glm::vec2(1.0f));
        shader->setVec2("uvOffset", glm::vec2(0.0f));
    }
}
