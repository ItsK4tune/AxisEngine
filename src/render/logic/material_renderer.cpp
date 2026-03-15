#include <render/logic/material_renderer.h>
#include <ecs/unit/render_components.h>
#include <render/logic/shader.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>

void MaterialRenderer::Initialize(IGraphicsContext& context, unsigned int whiteTextureId, unsigned int blackTextureId, unsigned int flatNormalTextureId) {
    m_Context = &context;
    m_WhiteTextureID = whiteTextureId;
    m_BlackTextureID = blackTextureId;
    m_FlatNormalTextureID = flatNormalTextureId;
}

bool MaterialRenderer::SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene, bool debugNoTexture) {
    bool boundSomething = false;
    if (scene.registry.all_of<MaterialComponent>(entity)) {
        auto &mat = scene.registry.get<MaterialComponent>(entity);
        if (mat.desc.type == MaterialType::PBR) {
            shader->setFloat("material.roughness", mat.desc.roughness);
            shader->setFloat("material.metallic", mat.desc.metallic);
            shader->setFloat("material.ao", mat.desc.ao);
            shader->setVec3("material.emission", mat.desc.emission);
            shader->setFloat("roughness", mat.desc.roughness);
            shader->setFloat("metallic", mat.desc.metallic);
            shader->setFloat("ao", mat.desc.ao);
            shader->setVec3("emission", mat.desc.emission);

            // Bind IBL textures if available from Skybox
            auto skyboxView = scene.registry.view<SkyboxRenderComponent>();
            for (auto skyEnt : skyboxView) {
                auto& skyComp = skyboxView.get<SkyboxRenderComponent>(skyEnt);
                if (skyComp.isPrimary && skyComp.skybox) {
                    auto& tm = m_Context->GetTextureManager();
                    if (skyComp.irradianceMap != 0) {
                        tm.ActiveTexture(TextureUnit::Texture6);
                        tm.BindTexture(TextureType::TextureCubeMap, skyComp.irradianceMap);
                        shader->setInt("irradianceMap", 6);
                    }
                    if (skyComp.prefilterMap != 0) {
                        tm.ActiveTexture(TextureUnit::Texture7);
                        tm.BindTexture(TextureType::TextureCubeMap, skyComp.prefilterMap);
                        shader->setInt("prefilterMap", 7);
                    }
                    if (skyComp.brdfLUT != 0) {
                        tm.ActiveTexture(TextureUnit::Texture8);
                        tm.BindTexture(TextureType::Texture2D, skyComp.brdfLUT);
                        shader->setInt("brdfLUT", 8);
                    }
                    break;
                }
            }
        } else {
            shader->setFloat("material.shininess", mat.desc.shininess);
            shader->setVec3("material.specular", mat.desc.specular);
            shader->setVec3("material.ambient", mat.desc.ambient);
            shader->setVec3("material.emission", mat.desc.emission);
            shader->setFloat("shininess", mat.desc.shininess);
            shader->setVec3("specular", mat.desc.specular);
            shader->setVec3("ambient", mat.desc.ambient);
            shader->setVec3("emission", mat.desc.emission);
            
            // A good estimate: high shininess => low roughness.
            // phong_shininess = 2 / roughness^4 - 2  => roughly
            float r = glm::clamp(1.0f - glm::sqrt(mat.desc.shininess / 128.0f), 0.05f, 1.0f);
            shader->setFloat("material.roughness", r);
            shader->setFloat("material.metallic", 0.0f);
            shader->setFloat("roughness", r);
            shader->setFloat("metallic", 0.0f);
        }
        shader->setFloat("material.opacity", mat.desc.opacity);
        shader->setFloat("opacity", mat.desc.opacity);
        shader->setVec2("uvScale", mat.desc.uvScale);
        shader->setVec2("uvOffset", mat.desc.uvOffset);
        shader->setCustomPorts(mat.desc.ports);

        if (debugNoTexture) {
            m_Context->GetTextureManager().ActiveTexture(TextureUnit::Texture0);
            m_Context->GetTextureManager().BindTexture(TextureType::Texture2D, m_WhiteTextureID);
            shader->setInt("material.texture_diffuse1", 0);
        } else {
            auto& tm = m_Context->GetTextureManager();
            if (mat.gpu.albedoMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture0);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.albedoMap);
                shader->setInt("material.texture_diffuse1", 0);
                shader->setInt("texture_diffuse1", 0);
                boundSomething = true;
            } else {
                tm.ActiveTexture(TextureUnit::Texture0);
                tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
                shader->setInt("material.texture_diffuse1", 0);
                shader->setInt("texture_diffuse1", 0);
            }

            if (mat.gpu.normalMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture1);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.normalMap);
                shader->setInt("material.texture_normal1", 1);
                shader->setInt("texture_normal1", 1);
            } else {
                tm.ActiveTexture(TextureUnit::Texture1);
                tm.BindTexture(TextureType::Texture2D, m_FlatNormalTextureID != 0 ? m_FlatNormalTextureID : m_WhiteTextureID);
                shader->setInt("material.texture_normal1", 1);
                shader->setInt("texture_normal1", 1);
            }

            if (mat.gpu.metallicMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture2);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.metallicMap);
                shader->setInt("material.texture_metallic1", 2);
                shader->setInt("texture_metallic1", 2);
            } else {
                tm.ActiveTexture(TextureUnit::Texture2);
                tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
                shader->setInt("material.texture_metallic1", 2);
                shader->setInt("texture_metallic1", 2);
            }

            if (mat.gpu.roughnessMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture3);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.roughnessMap);
                shader->setInt("material.texture_roughness1", 3);
                shader->setInt("texture_roughness1", 3);
            } else {
                tm.ActiveTexture(TextureUnit::Texture3);
                tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
                shader->setInt("material.texture_roughness1", 3);
                shader->setInt("texture_roughness1", 3);
            }

            if (mat.gpu.aoMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture4);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.aoMap);
                shader->setInt("material.texture_ao1", 4);
                shader->setInt("texture_ao1", 4);
            } else {
                tm.ActiveTexture(TextureUnit::Texture4);
                tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
                shader->setInt("material.texture_ao1", 4);
                shader->setInt("texture_ao1", 4);
            }

            if (mat.gpu.emissiveMap != 0) {
                tm.ActiveTexture(TextureUnit::Texture5);
                tm.BindTexture(TextureType::Texture2D, mat.gpu.emissiveMap);
                shader->setInt("material.texture_emissive1", 5);
                shader->setInt("texture_emissive1", 5);
            } else {
                tm.ActiveTexture(TextureUnit::Texture5);
                tm.BindTexture(TextureType::Texture2D, m_BlackTextureID != 0 ? m_BlackTextureID : m_WhiteTextureID);
                shader->setInt("material.texture_emissive1", 5);
                shader->setInt("texture_emissive1", 5);
            }
        }
        return boundSomething;
    } else {
        shader->setFloat("material.roughness", 0.5f);
        shader->setFloat("material.metallic", 0.0f);
        shader->setFloat("roughness", 0.5f);
        shader->setFloat("metallic", 0.0f);
        shader->setFloat("shininess", 32.0f);
        shader->setVec3("specular", glm::vec3(0.5f));
        shader->setVec3("ambient", glm::vec3(1.0f));
        shader->setVec3("emission", glm::vec3(0.0f));
        shader->setFloat("opacity", 1.0f);
        shader->setCustomPorts(ShaderPorts()); // Reset ports to 0
        
        m_Context->GetTextureManager().ActiveTexture(TextureUnit::Texture0);
        m_Context->GetTextureManager().BindTexture(TextureType::Texture2D, m_WhiteTextureID);
        shader->setInt("material.texture_diffuse1", 0);
        shader->setInt("texture_diffuse1", 0);
        
        for(int i = 1; i <= 5; ++i) {
            m_Context->GetTextureManager().ActiveTexture(static_cast<TextureUnit>(i));
            m_Context->GetTextureManager().BindTexture(TextureType::Texture2D, m_WhiteTextureID);
        }
        shader->setInt("material.texture_normal1", 1);
        shader->setInt("material.texture_metallic1", 2);
        shader->setInt("material.texture_roughness1", 3);
        shader->setInt("material.texture_ao1", 4);
        shader->setInt("material.texture_emissive1", 5);
        shader->setInt("texture_normal1", 1);
        shader->setInt("texture_metallic1", 2);
        shader->setInt("texture_roughness1", 3);
        shader->setInt("texture_ao1", 4);
        shader->setInt("texture_emissive1", 5);
        return false;
    }
}
