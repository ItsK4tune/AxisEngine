#pragma once

#include <glm/glm.hpp>
#include <render/type/render_data.h>
#include <unordered_map>
#include <shared_mutex>

class IGraphicsContext;
class Shader;
struct AxisMaterialComponent;

struct MaterialUniformLocations {
    int roughness = -1;
    int metallic = -1;
    int ao = -1;
    int emission = -1;
    int shininess = -1;
    int specular = -1;
    int ambient = -1;
    int opacity = -1;
    int uvScale = -1;
    int uvOffset = -1;
    int u_UVScale = -1;
    int u_UVOffset = -1;
    

    int mat_roughness = -1;
    int mat_metallic = -1;
    int mat_ao = -1;
    int mat_emission = -1;
    int mat_shininess = -1;
    int mat_specular = -1;
    int mat_ambient = -1;
    int mat_opacity = -1;

    int u_BaseColor = -1;
    int u_Metallic = -1;
    int u_Roughness = -1;
    int u_AO = -1;
    int u_Emission = -1;
    int u_Shininess = -1;
    int u_Specular = -1;


    int diffuseMap = -1;
    int normalMap = -1;
    int metallicMap = -1;
    int roughnessMap = -1;
    int aoMap = -1;
    int emissiveMap = -1;


    int mat_diffuseMap = -1;
    int mat_normalMap = -1;
    int mat_metallicMap = -1;
    int mat_roughnessMap = -1;
    int mat_aoMap = -1;
    int mat_emissiveMap = -1;

    int u_AlbedoMap = -1;
    int u_NormalMap = -1;
    int u_MetallicMap = -1;
    int u_RoughnessMap = -1;
    int u_AOMap = -1;
    int u_EmissiveMap = -1;
    int u_SpecularMap = -1;


    int irradianceMap = -1;
    int prefilterMap = -1;
    int brdfLUT = -1;
    int isWireframe = -1;

    bool initialized = false;
};

class MaterialRenderer {
public:
    void Initialize(IGraphicsContext* context, unsigned int whiteTextureId, unsigned int blackTextureId = 0, unsigned int flatNormalTextureId = 0);
    bool SetupMaterialUniforms(Shader *shader, AxisMaterialComponent* material, const RenderSceneData& sceneData, const glm::vec4& tintColor, bool debugNoTexture = false, bool isWireframe = false);

    static void InvalidateSkyboxCache() {
        std::lock_guard<std::mutex> lock(s_SkyboxMutex);
        s_SkyboxCache.valid = false;
    }

private:
    IGraphicsContext* m_Context = nullptr;
    unsigned int m_WhiteTextureID = 0;
    unsigned int m_BlackTextureID = 0;
    unsigned int m_FlatNormalTextureID = 0;


    mutable std::unordered_map<unsigned int, MaterialUniformLocations> m_LocationsCache;
    mutable std::shared_mutex m_CacheMutex;


    struct SkyboxCache {
        unsigned int irradianceMap = 0;
        unsigned int prefilterMap = 0;
        unsigned int brdfLUT = 0;
        bool valid = false;
    };
    static SkyboxCache s_SkyboxCache;
    static std::mutex s_SkyboxMutex;

    const MaterialUniformLocations& GetLocations(const Shader* shader) const;
};

