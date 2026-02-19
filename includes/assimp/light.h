



#pragma once
#ifndef AI_LIGHT_H_INC
#define AI_LIGHT_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif



enum aiLightSourceType {
    aiLightSource_UNDEFINED     = 0x0,

    
    
    
    aiLightSource_DIRECTIONAL   = 0x1,

    
    
    
    aiLightSource_POINT         = 0x2,

    
    
    
    
    aiLightSource_SPOT          = 0x3,

    
    
    
    
    
    aiLightSource_AMBIENT       = 0x4,

    
    
    
    aiLightSource_AREA          = 0x5,

    
#ifndef SWIG
    _aiLightSource_Force32Bit = INT_MAX
#endif
};



struct aiLight {
    
    C_STRUCT aiString mName;

    
    C_ENUM aiLightSourceType mType;

    
    C_STRUCT aiVector3D mPosition;

    
    C_STRUCT aiVector3D mDirection;

    
    C_STRUCT aiVector3D mUp;

    
    float mAttenuationConstant;

    
    float mAttenuationLinear;

    
    float mAttenuationQuadratic;

    
    C_STRUCT aiColor3D mColorDiffuse;

    
    C_STRUCT aiColor3D mColorSpecular;

    
    C_STRUCT aiColor3D mColorAmbient;

    
    float mAngleInnerCone;

    
    float mAngleOuterCone;

    
    C_STRUCT aiVector2D mSize;

#ifdef __cplusplus

    aiLight() AI_NO_EXCEPT
        :   mType                 (aiLightSource_UNDEFINED)
        ,   mAttenuationConstant  (0.f)
        ,   mAttenuationLinear    (1.f)
        ,   mAttenuationQuadratic (0.f)
        ,   mAngleInnerCone       ((float)AI_MATH_TWO_PI)
        ,   mAngleOuterCone       ((float)AI_MATH_TWO_PI)
        ,   mSize                 (0.f, 0.f)
    {
    }

#endif
};

#ifdef __cplusplus
}
#endif

#endif 
