#include <render/logic/material_renderer.h>
#include <ecs/unit/render_components.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <resource/logic/resource_manager.h>
#include <resource/unit/shader.h>

void MaterialRenderer::Initialize(IGraphicsContext* context, ResourceManager* resourceManager,
                                  unsigned int whiteTextureId, unsigned int blackTextureId,
                                  unsigned int flatNormalTextureId)
{
    m_Context = context;
    m_ResourceManager = resourceManager;
    m_WhiteTextureID = whiteTextureId;
    m_BlackTextureID = blackTextureId;
    m_FlatNormalTextureID = flatNormalTextureId;
    ResetTextureState();
}

const MaterialUniformLocations& MaterialRenderer::GetLocations(const Shader* shader) const
{
    unsigned int id = shader->getID();

    {
        std::shared_lock<std::shared_mutex> lock(m_CacheMutex);
        auto it = m_LocationsCache.find(id);
        if (it != m_LocationsCache.end())
            return it->second;
    }

    std::unique_lock<std::shared_mutex> lock(m_CacheMutex);
    auto& locs = m_LocationsCache[id];
    if (locs.initialized)
        return locs;

    locs.u_UVScale = shader->GetUniformLocation("u_UVScale");
    locs.u_UVOffset = shader->GetUniformLocation("u_UVOffset");

    locs.u_BaseColor = shader->GetUniformLocation("u_BaseColor");
    locs.u_Metallic = shader->GetUniformLocation("u_Metallic");
    locs.u_Roughness = shader->GetUniformLocation("u_Roughness");
    locs.u_AO = shader->GetUniformLocation("u_AO");
    locs.u_Emission = shader->GetUniformLocation("u_Emission");
    locs.u_Shininess = shader->GetUniformLocation("u_Shininess");
    locs.u_Specular = shader->GetUniformLocation("u_Specular");

    locs.u_AlbedoMap = shader->GetUniformLocation("u_AlbedoMap");
    locs.u_NormalMap = shader->GetUniformLocation("u_NormalMap");
    locs.u_MetallicMap = shader->GetUniformLocation("u_MetallicMap");
    locs.u_RoughnessMap = shader->GetUniformLocation("u_RoughnessMap");
    locs.u_AOMap = shader->GetUniformLocation("u_AOMap");
    locs.u_EmissiveMap = shader->GetUniformLocation("u_EmissiveMap");
    locs.u_SpecularMap = shader->GetUniformLocation("u_SpecularMap");
    locs.u_Lightmap = shader->GetUniformLocation("u_Lightmap");
    locs.u_LightmapIntensity = shader->GetUniformLocation("u_LightmapIntensity");

    locs.u_IrradianceMap = shader->GetUniformLocation("u_IrradianceMap");
    locs.u_PrefilterMap = shader->GetUniformLocation("u_PrefilterMap");
    locs.u_BrdfLUT = shader->GetUniformLocation("u_BrdfLUT");

    locs.initialized = true;
    return locs;
}

bool MaterialRenderer::SetupMaterialUniforms(Shader* shader, MaterialComponent* material,
                                             const RenderSceneData& sceneData, const glm::vec4& tintColor)
{
    if (!m_Context)
        return false;
    const auto& locs = GetLocations(shader);
    auto& tm = m_Context->GetTextureManager();

    bool boundSomething = false;
    if (material)
    {
        auto& mat = *material;

        if (mat.gpu.dirty && m_ResourceManager)
        {
            auto getTexID = [&](const std::string& path) -> uint32_t {
                if (path.empty())
                    return 0;
                auto tex = m_ResourceManager->GetTextureAuto(path);
                return tex ? tex->id : 0;
            };
            mat.gpu.albedoMap = getTexID(mat.desc.albedoPath);
            mat.gpu.normalMap = getTexID(mat.desc.normalPath);
            mat.gpu.metallicMap = getTexID(mat.desc.metallicPath);
            mat.gpu.roughnessMap = getTexID(mat.desc.roughnessPath);
            mat.gpu.aoMap = getTexID(mat.desc.aoPath);
            mat.gpu.emissiveMap = getTexID(mat.desc.emissivePath);
            mat.gpu.specularMap = getTexID(mat.desc.specularPath);
            mat.gpu.lightmapMap = getTexID(mat.desc.lightmapPath);
            mat.gpu.dirty = false;
        }

        boundSomething = !mat.desc.albedoPath.empty() || !mat.desc.normalPath.empty() ||
                         !mat.desc.metallicPath.empty() || !mat.desc.roughnessPath.empty() ||
                         !mat.desc.aoPath.empty() || !mat.desc.emissivePath.empty() || !mat.desc.specularPath.empty() ||
                         !mat.desc.lightmapPath.empty() ||
                         mat.gpu.albedoMap != 0 || mat.gpu.normalMap != 0 || mat.gpu.metallicMap != 0 ||
                         mat.gpu.roughnessMap != 0 || mat.gpu.aoMap != 0 || mat.gpu.emissiveMap != 0 ||
                         mat.gpu.specularMap != 0 || mat.gpu.lightmapMap != 0;
        if (locs.u_LightmapIntensity != -1)
            shader->setFloat(locs.u_LightmapIntensity,
                             mat.gpu.lightmapMap != 0 ? mat.desc.lightmapIntensity : 0.0f);

        if (locs.u_Roughness != -1)
            shader->setFloat(locs.u_Roughness, mat.desc.pbr.roughness);
        if (locs.u_Metallic != -1)
            shader->setFloat(locs.u_Metallic, mat.desc.pbr.metallic);
        if (locs.u_AO != -1)
            shader->setFloat(locs.u_AO, mat.desc.pbr.ao);
        if (locs.u_Emission != -1)
            shader->setVec3(locs.u_Emission, mat.desc.emission);
        if (locs.u_Shininess != -1)
        {
            float roughness = mat.desc.pbr.roughness;
            if (roughness < 0.0f)
                roughness = 0.0f;
            else if (roughness > 1.0f)
                roughness = 1.0f;
            shader->setFloat(locs.u_Shininess, 1.0f + (1.0f - roughness) * 127.0f);
        }
        if (locs.u_Specular != -1)
            shader->setVec3(locs.u_Specular, glm::vec3(0.5f));

        if (sceneData.irradianceMap != 0 && locs.u_IrradianceMap != -1)
        {
            if (m_LastBoundTextures[6] != sceneData.irradianceMap)
            {
                tm.ActiveTexture(TextureUnit::Texture6);
                tm.BindTexture(TextureType::TextureCubeMap, sceneData.irradianceMap);
                m_LastBoundTextures[6] = sceneData.irradianceMap;
            }
            shader->setInt(locs.u_IrradianceMap, 6);
        }
        if (sceneData.prefilterMap != 0 && locs.u_PrefilterMap != -1)
        {
            if (m_LastBoundTextures[7] != sceneData.prefilterMap)
            {
                tm.ActiveTexture(TextureUnit::Texture7);
                tm.BindTexture(TextureType::TextureCubeMap, sceneData.prefilterMap);
                m_LastBoundTextures[7] = sceneData.prefilterMap;
            }
            shader->setInt(locs.u_PrefilterMap, 7);
        }
        if (sceneData.brdfLUT != 0 && locs.u_BrdfLUT != -1)
        {
            if (m_LastBoundTextures[8] != sceneData.brdfLUT)
            {
                tm.ActiveTexture(TextureUnit::Texture8);
                tm.BindTexture(TextureType::Texture2D, sceneData.brdfLUT);
                m_LastBoundTextures[8] = sceneData.brdfLUT;
            }
            shader->setInt(locs.u_BrdfLUT, 8);
        }

        if (locs.u_BaseColor != -1)
        {
            glm::vec4 baseColor = tintColor;
            baseColor.a *= mat.desc.opacity;
            shader->setVec4(locs.u_BaseColor, baseColor);
        }

        if (locs.u_UVScale != -1)
            shader->setVec2(locs.u_UVScale, mat.desc.uvScale);
        if (locs.u_UVOffset != -1)
            shader->setVec2(locs.u_UVOffset, mat.desc.uvOffset);
        shader->setCustomPorts(mat.desc.ports);

        auto setTex = [&](int texUnit, unsigned int texID, int loc) {
            unsigned int targetTex =
                texID != 0 ? texID : ((texUnit == 5 || texUnit == 30) ? m_BlackTextureID : m_WhiteTextureID);
            if (m_LastBoundTextures[texUnit] != targetTex)
            {
                tm.ActiveTexture(static_cast<TextureUnit>(texUnit));
                tm.BindTexture(TextureType::Texture2D, targetTex);
                m_LastBoundTextures[texUnit] = targetTex;
            }
            if (loc != -1)
                shader->setInt(loc, texUnit);
        };

        setTex(0, mat.gpu.albedoMap, locs.u_AlbedoMap);
        if (mat.gpu.albedoMap != 0)
            boundSomething = true;

        setTex(1, mat.gpu.normalMap, locs.u_NormalMap);
        setTex(2, mat.gpu.metallicMap, locs.u_MetallicMap);
        setTex(3, mat.gpu.roughnessMap, locs.u_RoughnessMap);
        setTex(4, mat.gpu.aoMap, locs.u_AOMap);
        setTex(5, mat.gpu.emissiveMap, locs.u_EmissiveMap);
        setTex(9, mat.gpu.specularMap, locs.u_SpecularMap);
        setTex(30, mat.gpu.lightmapMap, locs.u_Lightmap);

        return boundSomething;
    }
    else
    {
        if (locs.u_BaseColor != -1)
        {
            shader->setVec4(locs.u_BaseColor, tintColor);
        }

        if (locs.u_Roughness != -1)
            shader->setFloat(locs.u_Roughness, 0.5f);
        if (locs.u_Metallic != -1)
            shader->setFloat(locs.u_Metallic, 0.0f);
        if (locs.u_Shininess != -1)
            shader->setFloat(locs.u_Shininess, 32.0f);
        if (locs.u_Specular != -1)
            shader->setVec3(locs.u_Specular, glm::vec3(0.5f));
        if (locs.u_Emission != -1)
            shader->setVec3(locs.u_Emission, glm::vec3(0.0f));
        if (locs.u_LightmapIntensity != -1)
            shader->setFloat(locs.u_LightmapIntensity, 0.0f);

        shader->setCustomPorts(ShaderPorts());

        if (m_LastBoundTextures[0] != m_WhiteTextureID)
        {
            tm.ActiveTexture(TextureUnit::Texture0);
            tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
            m_LastBoundTextures[0] = m_WhiteTextureID;
        }
        if (locs.u_AlbedoMap != -1)
            shader->setInt(locs.u_AlbedoMap, 0);

        for (int i = 1; i <= 5; ++i)
        {
            unsigned int target = (i == 5) ? m_BlackTextureID : m_WhiteTextureID;
            if (m_LastBoundTextures[i] != target)
            {
                tm.ActiveTexture(static_cast<TextureUnit>(i));
                tm.BindTexture(TextureType::Texture2D, target);
                m_LastBoundTextures[i] = target;
            }
        }
        if (locs.u_NormalMap != -1)
            shader->setInt(locs.u_NormalMap, 1);
        if (locs.u_MetallicMap != -1)
            shader->setInt(locs.u_MetallicMap, 2);
        if (locs.u_RoughnessMap != -1)
            shader->setInt(locs.u_RoughnessMap, 3);
        if (locs.u_AOMap != -1)
            shader->setInt(locs.u_AOMap, 4);
        if (locs.u_EmissiveMap != -1)
            shader->setInt(locs.u_EmissiveMap, 5);

        if (m_LastBoundTextures[9] != m_WhiteTextureID)
        {
            tm.ActiveTexture(TextureUnit::Texture9);
            tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
            m_LastBoundTextures[9] = m_WhiteTextureID;
        }
        if (locs.u_SpecularMap != -1)
            shader->setInt(locs.u_SpecularMap, 9);

        return false;
    }
}
