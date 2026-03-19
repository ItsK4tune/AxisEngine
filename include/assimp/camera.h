



#pragma once
#ifndef AI_CAMERA_H_INC
#define AI_CAMERA_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif



struct aiCamera {
    
    C_STRUCT aiString mName;

    
    C_STRUCT aiVector3D mPosition;

    
    C_STRUCT aiVector3D mUp;

    
    C_STRUCT aiVector3D mLookAt;

    
    float mHorizontalFOV;

    
    float mClipPlaneNear;

    
    float mClipPlaneFar;

    
    float mAspect;

    
    float mOrthographicWidth;
#ifdef __cplusplus

    aiCamera() AI_NO_EXCEPT
            : mUp(0.f, 1.f, 0.f),
              mLookAt(0.f, 0.f, 1.f),
              mHorizontalFOV(0.25f * (float)AI_MATH_PI),
              mClipPlaneNear(0.1f),
              mClipPlaneFar(1000.f),
              mAspect(0.f),
              mOrthographicWidth(0.f) {}

    
    void GetCameraMatrix(aiMatrix4x4 &out) const {
        

        
        aiVector3D zaxis = mLookAt;
        zaxis.Normalize();
        aiVector3D yaxis = mUp;
        yaxis.Normalize();
        aiVector3D xaxis = mUp ^ mLookAt;
        xaxis.Normalize();

        out.a4 = -(xaxis * mPosition);
        out.b4 = -(yaxis * mPosition);
        out.c4 = -(zaxis * mPosition);

        out.a1 = xaxis.x;
        out.a2 = xaxis.y;
        out.a3 = xaxis.z;

        out.b1 = yaxis.x;
        out.b2 = yaxis.y;
        out.b3 = yaxis.z;

        out.c1 = zaxis.x;
        out.c2 = zaxis.y;
        out.c3 = zaxis.z;

        out.d1 = out.d2 = out.d3 = 0.f;
        out.d4 = 1.f;
    }

#endif
};

#ifdef __cplusplus
}
#endif

#endif 
