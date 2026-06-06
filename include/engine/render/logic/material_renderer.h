#pragma once

#include <render/type/render_data.h>
#include <glm/glm.hpp>
#include <shared_mutex>
#include <unordered_map>

class IGraphicsContext;
class Shader;
struct MaterialComponent;

struct MaterialUniformLocations
{
    int u_UVScale = -1;
    int u_UVOffset = -1;

    int u_BaseColor = -1;
    int u_Metallic = -1;
    int u_Roughness = -1;
    int u_AO = -1;
    int u_Emission = -1;
    int u_Shininess = -1;
    int u_Specular = -1;

    int u_AlbedoMap = -1;
    int u_NormalMap = -1;
    int u_MetallicMap = -1;
    int u_RoughnessMap = -1;
    int u_AOMap = -1;
    int u_EmissiveMap = -1;
    int u_SpecularMap = -1;

    int u_IrradianceMap = -1;
    int u_PrefilterMap = -1;
    int u_BrdfLUT = -1;
    int u_isWireframe = -1;

    bool initialized = false;
};

class ResourceManager;

class MaterialRenderer
{
public:
    void Initialize(IGraphicsContext* context, ResourceManager* resourceManager, unsigned int whiteTextureId,
                    unsigned int blackTextureId = 0, unsigned int flatNormalTextureId = 0);
    bool SetupMaterialUniforms(Shader* shader, MaterialComponent* material, const RenderSceneData& sceneData,
                               const glm::vec4& tintColor, bool debugNoTexture = false, bool isWireframe = false);

    void ResetTextureState()
    {
        for (int i = 0; i < TextureStateSlots; ++i)
        {
            m_LastBoundTextures[i] = (unsigned int)-1;
        }
    }

    static void InvalidateSkyboxCache()
    {
        std::lock_guard<std::mutex> lock(s_SkyboxMutex);
        s_SkyboxCache.valid = false;
    }

private:
    struct SkyboxCache
    {
        unsigned int irradianceMap = 0;
        unsigned int prefilterMap = 0;
        unsigned int brdfLUT = 0;
        bool valid = false;
    };

    const MaterialUniformLocations& GetLocations(const Shader* shader) const;

    IGraphicsContext* m_Context = nullptr;
    ResourceManager* m_ResourceManager = nullptr;
    unsigned int m_WhiteTextureID = 0;
    unsigned int m_BlackTextureID = 0;
    unsigned int m_FlatNormalTextureID = 0;

    mutable std::unordered_map<unsigned int, MaterialUniformLocations> m_LocationsCache;
    mutable std::shared_mutex m_CacheMutex;

    static SkyboxCache s_SkyboxCache;
    static std::mutex s_SkyboxMutex;

    static constexpr int TextureStateSlots = 32;
    unsigned int m_LastBoundTextures[TextureStateSlots] = {0};
};
