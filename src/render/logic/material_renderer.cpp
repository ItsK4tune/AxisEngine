#include <render/logic/material_renderer.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/shader.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>

void MaterialRenderer::Initialize(IGraphicsContext& context, unsigned int whiteTextureId, unsigned int blackTextureId, unsigned int flatNormalTextureId) {
    m_Context = &context;
    m_WhiteTextureID = whiteTextureId;
    m_BlackTextureID = blackTextureId;
    m_FlatNormalTextureID = flatNormalTextureId;
}

MaterialRenderer::SkyboxCache MaterialRenderer::s_SkyboxCache;
std::mutex MaterialRenderer::s_SkyboxMutex;

const MaterialUniformLocations& MaterialRenderer::GetLocations(const Shader* shader) const {
    unsigned int id = shader->getID();
    
    {
        std::shared_lock<std::shared_mutex> lock(m_CacheMutex);
        auto it = m_LocationsCache.find(id);
        if (it != m_LocationsCache.end()) return it->second;
    }

    std::unique_lock<std::shared_mutex> lock(m_CacheMutex);
    auto& locs = m_LocationsCache[id];
    if (locs.initialized) return locs;

    locs.roughness = shader->GetUniformLocation("roughness");
    locs.metallic = shader->GetUniformLocation("metallic");
    locs.ao = shader->GetUniformLocation("ao");
    locs.emission = shader->GetUniformLocation("emission");
    locs.shininess = shader->GetUniformLocation("shininess");
    locs.specular = shader->GetUniformLocation("specular");
    locs.ambient = shader->GetUniformLocation("ambient");
    locs.opacity = shader->GetUniformLocation("opacity");
    locs.uvScale = shader->GetUniformLocation("uvScale");
    locs.uvOffset = shader->GetUniformLocation("uvOffset");

    locs.mat_roughness = shader->GetUniformLocation("material.roughness");
    locs.mat_metallic = shader->GetUniformLocation("material.metallic");
    locs.mat_ao = shader->GetUniformLocation("material.ao");
    locs.mat_emission = shader->GetUniformLocation("material.emission");
    locs.mat_shininess = shader->GetUniformLocation("material.shininess");
    locs.mat_specular = shader->GetUniformLocation("material.specular");
    locs.mat_ambient = shader->GetUniformLocation("material.ambient");
    locs.mat_opacity = shader->GetUniformLocation("material.opacity");

    locs.diffuseMap = shader->GetUniformLocation("texture_diffuse1");
    locs.normalMap = shader->GetUniformLocation("texture_normal1");
    locs.metallicMap = shader->GetUniformLocation("texture_metallic1");
    locs.roughnessMap = shader->GetUniformLocation("texture_roughness1");
    locs.aoMap = shader->GetUniformLocation("texture_ao1");
    locs.emissiveMap = shader->GetUniformLocation("texture_emissive1");

    locs.mat_diffuseMap = shader->GetUniformLocation("material.texture_diffuse1");
    locs.mat_normalMap = shader->GetUniformLocation("material.texture_normal1");
    locs.mat_metallicMap = shader->GetUniformLocation("material.texture_metallic1");
    locs.mat_roughnessMap = shader->GetUniformLocation("material.texture_roughness1");
    locs.mat_aoMap = shader->GetUniformLocation("material.texture_ao1");
    locs.mat_emissiveMap = shader->GetUniformLocation("material.texture_emissive1");

    locs.irradianceMap = shader->GetUniformLocation("irradianceMap");
    locs.prefilterMap = shader->GetUniformLocation("prefilterMap");
    locs.brdfLUT = shader->GetUniformLocation("brdfLUT");
    locs.isWireframe = shader->GetUniformLocation("u_isWireframe");

    locs.initialized = true;
    return locs;
}

bool MaterialRenderer::SetupMaterialUniforms(Shader *shader, MaterialComponent* material, const RenderSceneData& sceneData, bool debugNoTexture, bool isWireframe) {
    const auto& locs = GetLocations(shader);
    auto& tm = m_Context->GetTextureManager();

    if (locs.isWireframe != -1) shader->setBool(locs.isWireframe, isWireframe);

    bool boundSomething = false;
    if (material) {
        auto &mat = *material;
        if (mat.desc.type == MaterialType::PBR) {
            shader->setFloat(locs.mat_roughness, mat.desc.roughness);
            shader->setFloat(locs.mat_metallic, mat.desc.metallic);
            shader->setFloat(locs.mat_ao, mat.desc.ao);
            shader->setVec3(locs.mat_emission, mat.desc.emission);
            shader->setFloat(locs.roughness, mat.desc.roughness);
            shader->setFloat(locs.metallic, mat.desc.metallic);
            shader->setFloat(locs.ao, mat.desc.ao);
            shader->setVec3(locs.emission, mat.desc.emission);

            if (sceneData.irradianceMap != 0 && locs.irradianceMap != -1) {
                tm.ActiveTexture(TextureUnit::Texture6);
                tm.BindTexture(TextureType::TextureCubeMap, sceneData.irradianceMap);
                shader->setInt(locs.irradianceMap, 6);
            }
            if (sceneData.prefilterMap != 0 && locs.prefilterMap != -1) {
                tm.ActiveTexture(TextureUnit::Texture7);
                tm.BindTexture(TextureType::TextureCubeMap, sceneData.prefilterMap);
                shader->setInt(locs.prefilterMap, 7);
            }
            if (sceneData.brdfLUT != 0 && locs.brdfLUT != -1) {
                tm.ActiveTexture(TextureUnit::Texture8);
                tm.BindTexture(TextureType::Texture2D, sceneData.brdfLUT);
                shader->setInt(locs.brdfLUT, 8);
            }
        }
 else {
            shader->setFloat(locs.mat_shininess, mat.desc.shininess);
            shader->setVec3(locs.mat_specular, mat.desc.specular);
            shader->setVec3(locs.mat_ambient, mat.desc.ambient);
            shader->setVec3(locs.mat_emission, mat.desc.emission);
            shader->setFloat(locs.shininess, mat.desc.shininess);
            shader->setVec3(locs.specular, mat.desc.specular);
            shader->setVec3(locs.ambient, mat.desc.ambient);
            shader->setVec3(locs.emission, mat.desc.emission);
            
            float r = glm::clamp(1.0f - glm::sqrt(mat.desc.shininess / 128.0f), 0.05f, 1.0f);
            shader->setFloat(locs.mat_roughness, r);
            shader->setFloat(locs.mat_metallic, 0.0f);
            shader->setFloat(locs.roughness, r);
            shader->setFloat(locs.metallic, 0.0f);
        }
        shader->setFloat(locs.mat_opacity, mat.desc.opacity);
        shader->setFloat(locs.opacity, mat.desc.opacity);
        shader->setVec2(locs.uvScale, mat.desc.uvScale);
        shader->setVec2(locs.uvOffset, mat.desc.uvOffset);
        shader->setCustomPorts(mat.desc.ports);

        if (debugNoTexture) {
            tm.ActiveTexture(TextureUnit::Texture0);
            tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
            shader->setInt(locs.mat_diffuseMap, 0);
        } else {

            auto setTex = [&](int texUnit, unsigned int texID, int loc1, int loc2) {
                tm.ActiveTexture(static_cast<TextureUnit>(texUnit));
                tm.BindTexture(TextureType::Texture2D, texID != 0 ? texID : (texUnit == 5 ? m_BlackTextureID : m_WhiteTextureID));
                if (loc1 != -1) shader->setInt(loc1, texUnit);
                if (loc2 != -1) shader->setInt(loc2, texUnit);
            };

            setTex(0, mat.gpu.albedoMap, locs.mat_diffuseMap, locs.diffuseMap);
            if (mat.gpu.albedoMap != 0) boundSomething = true;
            
            setTex(1, mat.gpu.normalMap, locs.mat_normalMap, locs.normalMap);
            setTex(2, mat.gpu.metallicMap, locs.mat_metallicMap, locs.metallicMap);
            setTex(3, mat.gpu.roughnessMap, locs.mat_roughnessMap, locs.roughnessMap);
            setTex(4, mat.gpu.aoMap, locs.mat_aoMap, locs.aoMap);
            setTex(5, mat.gpu.emissiveMap, locs.mat_emissiveMap, locs.emissiveMap);
        }
        return boundSomething;
    } else {
        shader->setFloat(locs.mat_roughness, 0.5f);
        shader->setFloat(locs.mat_metallic, 0.0f);
        shader->setFloat(locs.roughness, 0.5f);
        shader->setFloat(locs.metallic, 0.0f);
        shader->setFloat(locs.shininess, 32.0f);
        shader->setVec3(locs.specular, glm::vec3(0.5f));
        shader->setVec3(locs.ambient, glm::vec3(1.0f));
        shader->setVec3(locs.emission, glm::vec3(0.0f));
        shader->setFloat(locs.opacity, 1.0f);
        shader->setCustomPorts(ShaderPorts());
        
        tm.ActiveTexture(TextureUnit::Texture0);
        tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
        shader->setInt(locs.mat_diffuseMap, 0);
        shader->setInt(locs.diffuseMap, 0);
        
        for(int i = 1; i <= 5; ++i) {
            tm.ActiveTexture(static_cast<TextureUnit>(i));
            tm.BindTexture(TextureType::Texture2D, i == 5 ? m_BlackTextureID : m_WhiteTextureID);
        }
        shader->setInt(locs.mat_normalMap, 1);
        shader->setInt(locs.mat_metallicMap, 2);
        shader->setInt(locs.mat_roughnessMap, 3);
        shader->setInt(locs.mat_aoMap, 4);
        shader->setInt(locs.mat_emissiveMap, 5);
        shader->setInt(locs.normalMap, 1);
        shader->setInt(locs.metallicMap, 2);
        shader->setInt(locs.roughnessMap, 3);
        shader->setInt(locs.aoMap, 4);
        shader->setInt(locs.emissiveMap, 5);
        return false;
    }
}


