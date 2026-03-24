#pragma once

#include <glm/glm.hpp>
#include <scene/logic/scene.h>
#include <unordered_map>
#include <shared_mutex>

class IGraphicsContext;
class Shader;
struct MaterialComponent;

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
    

    int mat_roughness = -1;
    int mat_metallic = -1;
    int mat_ao = -1;
    int mat_emission = -1;
    int mat_shininess = -1;
    int mat_specular = -1;
    int mat_ambient = -1;
    int mat_opacity = -1;


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


    int irradianceMap = -1;
    int prefilterMap = -1;
    int brdfLUT = -1;
    int isWireframe = -1;

    bool initialized = false;
};

class MaterialRenderer {
public:
    void Initialize(IGraphicsContext& context, unsigned int whiteTextureId, unsigned int blackTextureId = 0, unsigned int flatNormalTextureId = 0);
    bool SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene, bool debugNoTexture, bool isWireframe = false);
    bool SetupMaterialUniforms(Shader *shader, MaterialComponent* material, Scene &scene, bool debugNoTexture, bool isWireframe = false);

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

