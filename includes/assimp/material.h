


#pragma once
#ifndef AI_MATERIAL_H_INC
#define AI_MATERIAL_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define AI_DEFAULT_MATERIAL_NAME "DefaultMaterial"



enum aiTextureOp {
    
    aiTextureOp_Multiply = 0x0,

    
    aiTextureOp_Add = 0x1,

    
    aiTextureOp_Subtract = 0x2,

    
    aiTextureOp_Divide = 0x3,

    
    aiTextureOp_SmoothAdd = 0x4,

    
    aiTextureOp_SignedAdd = 0x5,

#ifndef SWIG
    _aiTextureOp_Force32Bit = INT_MAX
#endif
};



enum aiTextureMapMode {
    
    aiTextureMapMode_Wrap = 0x0,

    
    aiTextureMapMode_Clamp = 0x1,

    
    aiTextureMapMode_Decal = 0x3,

    
    aiTextureMapMode_Mirror = 0x2,

#ifndef SWIG
    _aiTextureMapMode_Force32Bit = INT_MAX
#endif
};



enum aiTextureMapping {
    
    aiTextureMapping_UV = 0x0,

    
    aiTextureMapping_SPHERE = 0x1,

    
    aiTextureMapping_CYLINDER = 0x2,

    
    aiTextureMapping_BOX = 0x3,

    
    aiTextureMapping_PLANE = 0x4,

    
    aiTextureMapping_OTHER = 0x5,

#ifndef SWIG
    _aiTextureMapping_Force32Bit = INT_MAX
#endif
};



enum aiTextureType {
    
    aiTextureType_NONE = 0,

    

    
    aiTextureType_DIFFUSE = 1,

    
    aiTextureType_SPECULAR = 2,

    
    aiTextureType_AMBIENT = 3,

    
    aiTextureType_EMISSIVE = 4,

    
    aiTextureType_HEIGHT = 5,

    
    aiTextureType_NORMALS = 6,

    
    aiTextureType_SHININESS = 7,

    
    aiTextureType_OPACITY = 8,

    
    aiTextureType_DISPLACEMENT = 9,

    
    aiTextureType_LIGHTMAP = 10,

    
    aiTextureType_REFLECTION = 11,

    

    aiTextureType_BASE_COLOR = 12,
    aiTextureType_NORMAL_CAMERA = 13,
    aiTextureType_EMISSION_COLOR = 14,
    aiTextureType_METALNESS = 15,
    aiTextureType_DIFFUSE_ROUGHNESS = 16,
    aiTextureType_AMBIENT_OCCLUSION = 17,

    
    aiTextureType_UNKNOWN = 18,

    

    
    aiTextureType_SHEEN = 19,

    
    aiTextureType_CLEARCOAT = 20,

    
    aiTextureType_TRANSMISSION = 21,

    
    aiTextureType_MAYA_BASE = 22,
    aiTextureType_MAYA_SPECULAR = 23,
    aiTextureType_MAYA_SPECULAR_COLOR = 24,
    aiTextureType_MAYA_SPECULAR_ROUGHNESS = 25,

    
    aiTextureType_ANISOTROPY = 26,

    
    aiTextureType_GLTF_METALLIC_ROUGHNESS = 27,

#ifndef SWIG
    _aiTextureType_Force32Bit = INT_MAX
#endif
};

#define AI_TEXTURE_TYPE_MAX aiTextureType_GLTF_METALLIC_ROUGHNESS



ASSIMP_API const char *aiTextureTypeToString(enum aiTextureType in);



enum aiShadingMode {
    
    aiShadingMode_Flat = 0x1,

    
    aiShadingMode_Gouraud = 0x2,

    
    aiShadingMode_Phong = 0x3,

    
    aiShadingMode_Blinn = 0x4,

    
    aiShadingMode_Toon = 0x5,

    
    aiShadingMode_OrenNayar = 0x6,

    
    aiShadingMode_Minnaert = 0x7,

    
    aiShadingMode_CookTorrance = 0x8,

    
    aiShadingMode_NoShading = 0x9,
    aiShadingMode_Unlit = aiShadingMode_NoShading, 

    
    aiShadingMode_Fresnel = 0xa,

    
    aiShadingMode_PBR_BRDF = 0xb,

#ifndef SWIG
    _aiShadingMode_Force32Bit = INT_MAX
#endif
};



enum aiTextureFlags {
    
    aiTextureFlags_Invert = 0x1,

    
    aiTextureFlags_UseAlpha = 0x2,

    
    aiTextureFlags_IgnoreAlpha = 0x4,

#ifndef SWIG
    _aiTextureFlags_Force32Bit = INT_MAX
#endif
};



enum aiBlendMode {
    
    aiBlendMode_Default = 0x0,

    
    aiBlendMode_Additive = 0x1,




#ifndef SWIG
    _aiBlendMode_Force32Bit = INT_MAX
#endif
};

#include "./Compiler/pushpack1.h"



struct aiUVTransform {
    
    C_STRUCT aiVector2D mTranslation;

    
    C_STRUCT aiVector2D mScaling;

    
    ai_real mRotation;

#ifdef __cplusplus
    aiUVTransform() AI_NO_EXCEPT
            : mTranslation(0.0, 0.0),
              mScaling(1.0, 1.0),
              mRotation(0.0) {
        
    }
#endif
};

#include "./Compiler/poppack1.h"




enum aiPropertyTypeInfo {
    
    aiPTI_Float = 0x1,

    
    aiPTI_Double = 0x2,

    
    aiPTI_String = 0x3,

    
    aiPTI_Integer = 0x4,

    
    aiPTI_Buffer = 0x5,


#ifndef SWIG
    _aiPTI_Force32Bit = INT_MAX
#endif
};



struct aiMaterialProperty {
    
    C_STRUCT aiString mKey;

    
    unsigned int mSemantic;

    
    unsigned int mIndex;

    
    unsigned int mDataLength;

    
    C_ENUM aiPropertyTypeInfo mType;

    
    char *mData;

#ifdef __cplusplus

    aiMaterialProperty() AI_NO_EXCEPT
            : mSemantic(0),
              mIndex(0),
              mDataLength(0),
              mType(aiPTI_Float),
              mData(nullptr) {
        
    }

    ~aiMaterialProperty() {
        delete[] mData;
        mData = nullptr;
    }

#endif
};


#ifdef __cplusplus
} 
#endif



#ifdef __cplusplus
struct ASSIMP_API aiMaterial
#else
struct aiMaterial
#endif
{

#ifdef __cplusplus

public:
    
    aiMaterial();

    
    ~aiMaterial();

    
    
    
    aiString GetName() const;

    
    
    template <typename Type>
    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, Type *pOut, unsigned int *pMax) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, int *pOut, unsigned int *pMax) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, ai_real *pOut, unsigned int *pMax) const;

    
    
    template <typename Type>
    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, Type &pOut) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, int &pOut) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, ai_real &pOut) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, aiString &pOut) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, aiColor3D &pOut) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, aiColor4D &pOut) const;

    aiReturn Get(const char *pKey, unsigned int type,
            unsigned int idx, aiUVTransform &pOut) const;

    
    
    unsigned int GetTextureCount(aiTextureType type) const;

    
    
    
    aiReturn GetTexture(aiTextureType type,
            unsigned int index,
            C_STRUCT aiString *path,
            aiTextureMapping *mapping = NULL,
            unsigned int *uvindex = NULL,
            ai_real *blend = NULL,
            aiTextureOp *op = NULL,
            aiTextureMapMode *mapmode = NULL) const;

    

    
    
    aiReturn AddBinaryProperty(const void *pInput,
            unsigned int pSizeInBytes,
            const char *pKey,
            unsigned int type,
            unsigned int index,
            aiPropertyTypeInfo pType);

    
    
    aiReturn AddProperty(const aiString *pInput,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    
    
    template <class TYPE>
    aiReturn AddProperty(const TYPE *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    aiReturn AddProperty(const aiVector3D *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    aiReturn AddProperty(const aiColor3D *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    aiReturn AddProperty(const aiColor4D *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    aiReturn AddProperty(const int *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    aiReturn AddProperty(const float *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    aiReturn AddProperty(const double *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    aiReturn AddProperty(const aiUVTransform *pInput,
            unsigned int pNumValues,
            const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    
    
    aiReturn RemoveProperty(const char *pKey,
            unsigned int type = 0,
            unsigned int index = 0);

    
    
    void Clear();

    
    
    static void CopyPropertyList(aiMaterial *pcDest,
            const aiMaterial *pcSrc);

#endif

    
    C_STRUCT aiMaterialProperty **mProperties;

    
    unsigned int mNumProperties;

    
    unsigned int mNumAllocated;
};


#ifdef __cplusplus
extern "C" {
#endif


#define AI_MATKEY_NAME "?mat.name", 0, 0
#define AI_MATKEY_TWOSIDED "$mat.twosided", 0, 0
#define AI_MATKEY_SHADING_MODEL "$mat.shadingm", 0, 0
#define AI_MATKEY_ENABLE_WIREFRAME "$mat.wireframe", 0, 0
#define AI_MATKEY_BLEND_FUNC "$mat.blend", 0, 0
#define AI_MATKEY_OPACITY "$mat.opacity", 0, 0
#define AI_MATKEY_TRANSPARENCYFACTOR "$mat.transparencyfactor", 0, 0
#define AI_MATKEY_BUMPSCALING "$mat.bumpscaling", 0, 0
#define AI_MATKEY_SHININESS "$mat.shininess", 0, 0
#define AI_MATKEY_REFLECTIVITY "$mat.reflectivity", 0, 0
#define AI_MATKEY_SHININESS_STRENGTH "$mat.shinpercent", 0, 0
#define AI_MATKEY_REFRACTI "$mat.refracti", 0, 0
#define AI_MATKEY_COLOR_DIFFUSE "$clr.diffuse", 0, 0
#define AI_MATKEY_COLOR_AMBIENT "$clr.ambient", 0, 0
#define AI_MATKEY_COLOR_SPECULAR "$clr.specular", 0, 0
#define AI_MATKEY_COLOR_EMISSIVE "$clr.emissive", 0, 0
#define AI_MATKEY_COLOR_TRANSPARENT "$clr.transparent", 0, 0
#define AI_MATKEY_COLOR_REFLECTIVE "$clr.reflective", 0, 0
#define AI_MATKEY_GLOBAL_BACKGROUND_IMAGE "?bg.global", 0, 0
#define AI_MATKEY_GLOBAL_SHADERLANG "?sh.lang", 0, 0
#define AI_MATKEY_SHADER_VERTEX "?sh.vs", 0, 0
#define AI_MATKEY_SHADER_FRAGMENT "?sh.fs", 0, 0
#define AI_MATKEY_SHADER_GEO "?sh.gs", 0, 0
#define AI_MATKEY_SHADER_TESSELATION "?sh.ts", 0, 0
#define AI_MATKEY_SHADER_PRIMITIVE "?sh.ps", 0, 0
#define AI_MATKEY_SHADER_COMPUTE "?sh.cs", 0, 0





#define AI_MATKEY_USE_COLOR_MAP "$mat.useColorMap", 0, 0






#define AI_MATKEY_BASE_COLOR "$clr.base", 0, 0
#define AI_MATKEY_BASE_COLOR_TEXTURE aiTextureType_BASE_COLOR, 0
#define AI_MATKEY_USE_METALLIC_MAP "$mat.useMetallicMap", 0, 0

#define AI_MATKEY_METALLIC_FACTOR "$mat.metallicFactor", 0, 0
#define AI_MATKEY_METALLIC_TEXTURE aiTextureType_METALNESS, 0
#define AI_MATKEY_USE_ROUGHNESS_MAP "$mat.useRoughnessMap", 0, 0

#define AI_MATKEY_ROUGHNESS_FACTOR "$mat.roughnessFactor", 0, 0
#define AI_MATKEY_ROUGHNESS_TEXTURE aiTextureType_DIFFUSE_ROUGHNESS, 0


#define AI_MATKEY_ANISOTROPY_FACTOR "$mat.anisotropyFactor", 0, 0








#define AI_MATKEY_SPECULAR_FACTOR "$mat.specularFactor", 0, 0

#define AI_MATKEY_GLOSSINESS_FACTOR "$mat.glossinessFactor", 0, 0




#define AI_MATKEY_SHEEN_COLOR_FACTOR "$clr.sheen.factor", 0, 0

#define AI_MATKEY_SHEEN_ROUGHNESS_FACTOR "$mat.sheen.roughnessFactor", 0, 0
#define AI_MATKEY_SHEEN_COLOR_TEXTURE aiTextureType_SHEEN, 0
#define AI_MATKEY_SHEEN_ROUGHNESS_TEXTURE aiTextureType_SHEEN, 1




#define AI_MATKEY_CLEARCOAT_FACTOR           "$mat.clearcoat.factor", 0, 0
#define AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR "$mat.clearcoat.roughnessFactor", 0, 0
#define AI_MATKEY_CLEARCOAT_TEXTURE aiTextureType_CLEARCOAT, 0
#define AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE aiTextureType_CLEARCOAT, 1
#define AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE aiTextureType_CLEARCOAT, 2





#define AI_MATKEY_TRANSMISSION_FACTOR "$mat.transmission.factor", 0, 0


#define AI_MATKEY_TRANSMISSION_TEXTURE aiTextureType_TRANSMISSION, 0





#define AI_MATKEY_VOLUME_THICKNESS_FACTOR "$mat.volume.thicknessFactor", 0, 0


#define AI_MATKEY_VOLUME_THICKNESS_TEXTURE aiTextureType_TRANSMISSION, 1

#define AI_MATKEY_VOLUME_ATTENUATION_DISTANCE "$mat.volume.attenuationDistance", 0, 0

#define AI_MATKEY_VOLUME_ATTENUATION_COLOR "$mat.volume.attenuationColor", 0, 0



#define AI_MATKEY_USE_EMISSIVE_MAP   "$mat.useEmissiveMap", 0, 0
#define AI_MATKEY_EMISSIVE_INTENSITY "$mat.emissiveIntensity", 0, 0
#define AI_MATKEY_USE_AO_MAP         "$mat.useAOMap", 0, 0



#define AI_MATKEY_ANISOTROPY_ROTATION "$mat.anisotropyRotation", 0, 0
#define AI_MATKEY_ANISOTROPY_TEXTURE aiTextureType_ANISOTROPY, 0




#define _AI_MATKEY_TEXTURE_BASE       "$tex.file"
#define _AI_MATKEY_UVWSRC_BASE        "$tex.uvwsrc"
#define _AI_MATKEY_TEXOP_BASE         "$tex.op"
#define _AI_MATKEY_MAPPING_BASE       "$tex.mapping"
#define _AI_MATKEY_TEXBLEND_BASE      "$tex.blend"
#define _AI_MATKEY_MAPPINGMODE_U_BASE "$tex.mapmodeu"
#define _AI_MATKEY_MAPPINGMODE_V_BASE "$tex.mapmodev"
#define _AI_MATKEY_TEXMAP_AXIS_BASE   "$tex.mapaxis"
#define _AI_MATKEY_UVTRANSFORM_BASE   "$tex.uvtrafo"
#define _AI_MATKEY_TEXFLAGS_BASE      "$tex.flags"



#define AI_MATKEY_TEXTURE(type, N) _AI_MATKEY_TEXTURE_BASE, type, N



#define AI_MATKEY_TEXTURE_DIFFUSE(N) \
    AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_TEXTURE_SPECULAR(N) \
    AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, N)

#define AI_MATKEY_TEXTURE_AMBIENT(N) \
    AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, N)

#define AI_MATKEY_TEXTURE_EMISSIVE(N) \
    AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_TEXTURE_NORMALS(N) \
    AI_MATKEY_TEXTURE(aiTextureType_NORMALS, N)

#define AI_MATKEY_TEXTURE_HEIGHT(N) \
    AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, N)

#define AI_MATKEY_TEXTURE_SHININESS(N) \
    AI_MATKEY_TEXTURE(aiTextureType_SHININESS, N)

#define AI_MATKEY_TEXTURE_OPACITY(N) \
    AI_MATKEY_TEXTURE(aiTextureType_OPACITY, N)

#define AI_MATKEY_TEXTURE_DISPLACEMENT(N) \
    AI_MATKEY_TEXTURE(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_TEXTURE_LIGHTMAP(N) \
    AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_TEXTURE_REFLECTION(N) \
    AI_MATKEY_TEXTURE(aiTextureType_REFLECTION, N)




#define AI_MATKEY_UVWSRC(type, N) _AI_MATKEY_UVWSRC_BASE, type, N



#define AI_MATKEY_UVWSRC_DIFFUSE(N) \
    AI_MATKEY_UVWSRC(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_UVWSRC_SPECULAR(N) \
    AI_MATKEY_UVWSRC(aiTextureType_SPECULAR, N)

#define AI_MATKEY_UVWSRC_AMBIENT(N) \
    AI_MATKEY_UVWSRC(aiTextureType_AMBIENT, N)

#define AI_MATKEY_UVWSRC_EMISSIVE(N) \
    AI_MATKEY_UVWSRC(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_UVWSRC_NORMALS(N) \
    AI_MATKEY_UVWSRC(aiTextureType_NORMALS, N)

#define AI_MATKEY_UVWSRC_HEIGHT(N) \
    AI_MATKEY_UVWSRC(aiTextureType_HEIGHT, N)

#define AI_MATKEY_UVWSRC_SHININESS(N) \
    AI_MATKEY_UVWSRC(aiTextureType_SHININESS, N)

#define AI_MATKEY_UVWSRC_OPACITY(N) \
    AI_MATKEY_UVWSRC(aiTextureType_OPACITY, N)

#define AI_MATKEY_UVWSRC_DISPLACEMENT(N) \
    AI_MATKEY_UVWSRC(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_UVWSRC_LIGHTMAP(N) \
    AI_MATKEY_UVWSRC(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_UVWSRC_REFLECTION(N) \
    AI_MATKEY_UVWSRC(aiTextureType_REFLECTION, N)



#define AI_MATKEY_TEXOP(type, N) _AI_MATKEY_TEXOP_BASE, type, N



#define AI_MATKEY_TEXOP_DIFFUSE(N) \
    AI_MATKEY_TEXOP(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_TEXOP_SPECULAR(N) \
    AI_MATKEY_TEXOP(aiTextureType_SPECULAR, N)

#define AI_MATKEY_TEXOP_AMBIENT(N) \
    AI_MATKEY_TEXOP(aiTextureType_AMBIENT, N)

#define AI_MATKEY_TEXOP_EMISSIVE(N) \
    AI_MATKEY_TEXOP(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_TEXOP_NORMALS(N) \
    AI_MATKEY_TEXOP(aiTextureType_NORMALS, N)

#define AI_MATKEY_TEXOP_HEIGHT(N) \
    AI_MATKEY_TEXOP(aiTextureType_HEIGHT, N)

#define AI_MATKEY_TEXOP_SHININESS(N) \
    AI_MATKEY_TEXOP(aiTextureType_SHININESS, N)

#define AI_MATKEY_TEXOP_OPACITY(N) \
    AI_MATKEY_TEXOP(aiTextureType_OPACITY, N)

#define AI_MATKEY_TEXOP_DISPLACEMENT(N) \
    AI_MATKEY_TEXOP(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_TEXOP_LIGHTMAP(N) \
    AI_MATKEY_TEXOP(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_TEXOP_REFLECTION(N) \
    AI_MATKEY_TEXOP(aiTextureType_REFLECTION, N)



#define AI_MATKEY_MAPPING(type, N) _AI_MATKEY_MAPPING_BASE, type, N



#define AI_MATKEY_MAPPING_DIFFUSE(N) \
    AI_MATKEY_MAPPING(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_MAPPING_SPECULAR(N) \
    AI_MATKEY_MAPPING(aiTextureType_SPECULAR, N)

#define AI_MATKEY_MAPPING_AMBIENT(N) \
    AI_MATKEY_MAPPING(aiTextureType_AMBIENT, N)

#define AI_MATKEY_MAPPING_EMISSIVE(N) \
    AI_MATKEY_MAPPING(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_MAPPING_NORMALS(N) \
    AI_MATKEY_MAPPING(aiTextureType_NORMALS, N)

#define AI_MATKEY_MAPPING_HEIGHT(N) \
    AI_MATKEY_MAPPING(aiTextureType_HEIGHT, N)

#define AI_MATKEY_MAPPING_SHININESS(N) \
    AI_MATKEY_MAPPING(aiTextureType_SHININESS, N)

#define AI_MATKEY_MAPPING_OPACITY(N) \
    AI_MATKEY_MAPPING(aiTextureType_OPACITY, N)

#define AI_MATKEY_MAPPING_DISPLACEMENT(N) \
    AI_MATKEY_MAPPING(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_MAPPING_LIGHTMAP(N) \
    AI_MATKEY_MAPPING(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_MAPPING_REFLECTION(N) \
    AI_MATKEY_MAPPING(aiTextureType_REFLECTION, N)



#define AI_MATKEY_TEXBLEND(type, N) _AI_MATKEY_TEXBLEND_BASE, type, N



#define AI_MATKEY_TEXBLEND_DIFFUSE(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_TEXBLEND_SPECULAR(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_SPECULAR, N)

#define AI_MATKEY_TEXBLEND_AMBIENT(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_AMBIENT, N)

#define AI_MATKEY_TEXBLEND_EMISSIVE(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_TEXBLEND_NORMALS(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_NORMALS, N)

#define AI_MATKEY_TEXBLEND_HEIGHT(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_HEIGHT, N)

#define AI_MATKEY_TEXBLEND_SHININESS(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_SHININESS, N)

#define AI_MATKEY_TEXBLEND_OPACITY(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_OPACITY, N)

#define AI_MATKEY_TEXBLEND_DISPLACEMENT(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_TEXBLEND_LIGHTMAP(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_TEXBLEND_REFLECTION(N) \
    AI_MATKEY_TEXBLEND(aiTextureType_REFLECTION, N)



#define AI_MATKEY_MAPPINGMODE_U(type, N) _AI_MATKEY_MAPPINGMODE_U_BASE, type, N



#define AI_MATKEY_MAPPINGMODE_U_DIFFUSE(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_MAPPINGMODE_U_SPECULAR(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_SPECULAR, N)

#define AI_MATKEY_MAPPINGMODE_U_AMBIENT(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_AMBIENT, N)

#define AI_MATKEY_MAPPINGMODE_U_EMISSIVE(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_MAPPINGMODE_U_NORMALS(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_NORMALS, N)

#define AI_MATKEY_MAPPINGMODE_U_HEIGHT(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_HEIGHT, N)

#define AI_MATKEY_MAPPINGMODE_U_SHININESS(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_SHININESS, N)

#define AI_MATKEY_MAPPINGMODE_U_OPACITY(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_OPACITY, N)

#define AI_MATKEY_MAPPINGMODE_U_DISPLACEMENT(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_MAPPINGMODE_U_LIGHTMAP(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_MAPPINGMODE_U_REFLECTION(N) \
    AI_MATKEY_MAPPINGMODE_U(aiTextureType_REFLECTION, N)



#define AI_MATKEY_MAPPINGMODE_V(type, N) _AI_MATKEY_MAPPINGMODE_V_BASE, type, N



#define AI_MATKEY_MAPPINGMODE_V_DIFFUSE(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_MAPPINGMODE_V_SPECULAR(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_SPECULAR, N)

#define AI_MATKEY_MAPPINGMODE_V_AMBIENT(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_AMBIENT, N)

#define AI_MATKEY_MAPPINGMODE_V_EMISSIVE(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_MAPPINGMODE_V_NORMALS(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_NORMALS, N)

#define AI_MATKEY_MAPPINGMODE_V_HEIGHT(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_HEIGHT, N)

#define AI_MATKEY_MAPPINGMODE_V_SHININESS(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_SHININESS, N)

#define AI_MATKEY_MAPPINGMODE_V_OPACITY(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_OPACITY, N)

#define AI_MATKEY_MAPPINGMODE_V_DISPLACEMENT(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_MAPPINGMODE_V_LIGHTMAP(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_MAPPINGMODE_V_REFLECTION(N) \
    AI_MATKEY_MAPPINGMODE_V(aiTextureType_REFLECTION, N)



#define AI_MATKEY_TEXMAP_AXIS(type, N) _AI_MATKEY_TEXMAP_AXIS_BASE, type, N



#define AI_MATKEY_TEXMAP_AXIS_DIFFUSE(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_TEXMAP_AXIS_SPECULAR(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_SPECULAR, N)

#define AI_MATKEY_TEXMAP_AXIS_AMBIENT(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_AMBIENT, N)

#define AI_MATKEY_TEXMAP_AXIS_EMISSIVE(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_TEXMAP_AXIS_NORMALS(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_NORMALS, N)

#define AI_MATKEY_TEXMAP_AXIS_HEIGHT(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_HEIGHT, N)

#define AI_MATKEY_TEXMAP_AXIS_SHININESS(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_SHININESS, N)

#define AI_MATKEY_TEXMAP_AXIS_OPACITY(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_OPACITY, N)

#define AI_MATKEY_TEXMAP_AXIS_DISPLACEMENT(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_TEXMAP_AXIS_LIGHTMAP(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_TEXMAP_AXIS_REFLECTION(N) \
    AI_MATKEY_TEXMAP_AXIS(aiTextureType_REFLECTION, N)



#define AI_MATKEY_UVTRANSFORM(type, N) _AI_MATKEY_UVTRANSFORM_BASE, type, N



#define AI_MATKEY_UVTRANSFORM_DIFFUSE(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_UVTRANSFORM_SPECULAR(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_SPECULAR, N)

#define AI_MATKEY_UVTRANSFORM_AMBIENT(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_AMBIENT, N)

#define AI_MATKEY_UVTRANSFORM_EMISSIVE(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_UVTRANSFORM_NORMALS(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_NORMALS, N)

#define AI_MATKEY_UVTRANSFORM_HEIGHT(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_HEIGHT, N)

#define AI_MATKEY_UVTRANSFORM_SHININESS(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_SHININESS, N)

#define AI_MATKEY_UVTRANSFORM_OPACITY(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_OPACITY, N)

#define AI_MATKEY_UVTRANSFORM_DISPLACEMENT(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_UVTRANSFORM_LIGHTMAP(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_UVTRANSFORM_REFLECTION(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_REFLECTION, N)

#define AI_MATKEY_UVTRANSFORM_UNKNOWN(N) \
    AI_MATKEY_UVTRANSFORM(aiTextureType_UNKNOWN, N)



#define AI_MATKEY_TEXFLAGS(type, N) _AI_MATKEY_TEXFLAGS_BASE, type, N



#define AI_MATKEY_TEXFLAGS_DIFFUSE(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_DIFFUSE, N)

#define AI_MATKEY_TEXFLAGS_SPECULAR(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_SPECULAR, N)

#define AI_MATKEY_TEXFLAGS_AMBIENT(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_AMBIENT, N)

#define AI_MATKEY_TEXFLAGS_EMISSIVE(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_EMISSIVE, N)

#define AI_MATKEY_TEXFLAGS_NORMALS(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_NORMALS, N)

#define AI_MATKEY_TEXFLAGS_HEIGHT(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_HEIGHT, N)

#define AI_MATKEY_TEXFLAGS_SHININESS(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_SHININESS, N)

#define AI_MATKEY_TEXFLAGS_OPACITY(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_OPACITY, N)

#define AI_MATKEY_TEXFLAGS_DISPLACEMENT(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_DISPLACEMENT, N)

#define AI_MATKEY_TEXFLAGS_LIGHTMAP(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_LIGHTMAP, N)

#define AI_MATKEY_TEXFLAGS_REFLECTION(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_REFLECTION, N)

#define AI_MATKEY_TEXFLAGS_UNKNOWN(N) \
    AI_MATKEY_TEXFLAGS(aiTextureType_UNKNOWN, N)






ASSIMP_API C_ENUM aiReturn aiGetMaterialProperty(
        const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        const C_STRUCT aiMaterialProperty **pPropOut);




ASSIMP_API C_ENUM aiReturn aiGetMaterialFloatArray(
        const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        ai_real *pOut,
        unsigned int *pMax);




static inline aiReturn aiGetMaterialFloat(const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        ai_real *pOut) {
    return aiGetMaterialFloatArray(pMat, pKey, type, index, pOut, NULL);
}



ASSIMP_API C_ENUM aiReturn aiGetMaterialIntegerArray(const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        int *pOut,
        unsigned int *pMax);




static inline aiReturn aiGetMaterialInteger(const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        int *pOut) {
    return aiGetMaterialIntegerArray(pMat, pKey, type, index, pOut, NULL);
}




ASSIMP_API C_ENUM aiReturn aiGetMaterialColor(const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        C_STRUCT aiColor4D *pOut);




ASSIMP_API C_ENUM aiReturn aiGetMaterialUVTransform(const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        C_STRUCT aiUVTransform *pOut);




ASSIMP_API C_ENUM aiReturn aiGetMaterialString(const C_STRUCT aiMaterial *pMat,
        const char *pKey,
        unsigned int type,
        unsigned int index,
        C_STRUCT aiString *pOut);




ASSIMP_API unsigned int aiGetMaterialTextureCount(const C_STRUCT aiMaterial *pMat,
        C_ENUM aiTextureType type);




#ifdef __cplusplus
ASSIMP_API aiReturn aiGetMaterialTexture(const C_STRUCT aiMaterial *mat,
        aiTextureType type,
        unsigned int index,
        aiString *path,
        aiTextureMapping *mapping = NULL,
        unsigned int *uvindex = NULL,
        ai_real *blend = NULL,
        aiTextureOp *op = NULL,
        aiTextureMapMode *mapmode = NULL,
        unsigned int *flags = NULL);
#else
C_ENUM aiReturn aiGetMaterialTexture(const C_STRUCT aiMaterial *mat,
        C_ENUM aiTextureType type,
        unsigned int index,
        C_STRUCT aiString *path,
        C_ENUM aiTextureMapping *mapping ,
        unsigned int *uvindex ,
        ai_real *blend ,
        C_ENUM aiTextureOp *op ,
        C_ENUM aiTextureMapMode *mapmode ,
        unsigned int *flags );
#endif 

#ifdef __cplusplus
}

#include "material.inl"

#endif 

#endif 
