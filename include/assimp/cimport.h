


#pragma once
#ifndef AI_ASSIMP_H_INC
#define AI_ASSIMP_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/importerdesc.h>
#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct aiScene;
struct aiTexture;
struct aiFileIO;

typedef void (*aiLogStreamCallback)(const char * , char * );




struct aiLogStream {
    
    aiLogStreamCallback callback;

    
    char *user;
};




struct aiPropertyStore {
    char sentinel;
};


typedef int aiBool;

#define AI_FALSE 0
#define AI_TRUE 1



ASSIMP_API const C_STRUCT aiScene *aiImportFile(
        const char *pFile,
        unsigned int pFlags);



ASSIMP_API const C_STRUCT aiScene *aiImportFileEx(
        const char *pFile,
        unsigned int pFlags,
        C_STRUCT aiFileIO *pFS);



ASSIMP_API const C_STRUCT aiScene *aiImportFileExWithProperties(
        const char *pFile,
        unsigned int pFlags,
        C_STRUCT aiFileIO *pFS,
        const C_STRUCT aiPropertyStore *pProps);



ASSIMP_API const C_STRUCT aiScene *aiImportFileFromMemory(
        const char *pBuffer,
        unsigned int pLength,
        unsigned int pFlags,
        const char *pHint);



ASSIMP_API const C_STRUCT aiScene *aiImportFileFromMemoryWithProperties(
        const char *pBuffer,
        unsigned int pLength,
        unsigned int pFlags,
        const char *pHint,
        const C_STRUCT aiPropertyStore *pProps);



ASSIMP_API const C_STRUCT aiScene *aiApplyPostProcessing(
        const C_STRUCT aiScene *pScene,
        unsigned int pFlags);



ASSIMP_API C_STRUCT aiLogStream aiGetPredefinedLogStream(
        C_ENUM aiDefaultLogStream pStreams,
        const char *file);



ASSIMP_API void aiAttachLogStream(
        const C_STRUCT aiLogStream *stream);



ASSIMP_API void aiEnableVerboseLogging(aiBool d);



ASSIMP_API C_ENUM aiReturn aiDetachLogStream(
        const C_STRUCT aiLogStream *stream);



ASSIMP_API void aiDetachAllLogStreams(void);



ASSIMP_API void aiReleaseImport(
        const C_STRUCT aiScene *pScene);



ASSIMP_API const char *aiGetErrorString(void);



ASSIMP_API aiBool aiIsExtensionSupported(
        const char *szExtension);



ASSIMP_API void aiGetExtensionList(
        C_STRUCT aiString *szOut);



ASSIMP_API void aiGetMemoryRequirements(
        const C_STRUCT aiScene *pIn,
        C_STRUCT aiMemoryInfo *in);



ASSIMP_API const C_STRUCT aiTexture *aiGetEmbeddedTexture(const C_STRUCT aiScene *pIn, const char *filename);



ASSIMP_API C_STRUCT aiPropertyStore *aiCreatePropertyStore(void);



ASSIMP_API void aiReleasePropertyStore(C_STRUCT aiPropertyStore *p);



ASSIMP_API void aiSetImportPropertyInteger(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        int value);



ASSIMP_API void aiSetImportPropertyFloat(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        ai_real value);



ASSIMP_API void aiSetImportPropertyString(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        const C_STRUCT aiString *st);



ASSIMP_API void aiSetImportPropertyMatrix(
        C_STRUCT aiPropertyStore *store,
        const char *szName,
        const C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API void aiCreateQuaternionFromMatrix(
        C_STRUCT aiQuaternion *quat,
        const C_STRUCT aiMatrix3x3 *mat);



ASSIMP_API void aiDecomposeMatrix(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiVector3D *scaling,
        C_STRUCT aiQuaternion *rotation,
        C_STRUCT aiVector3D *position);



ASSIMP_API void aiTransposeMatrix4(
        C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API void aiTransposeMatrix3(
        C_STRUCT aiMatrix3x3 *mat);



ASSIMP_API void aiTransformVecByMatrix3(
        C_STRUCT aiVector3D *vec,
        const C_STRUCT aiMatrix3x3 *mat);



ASSIMP_API void aiTransformVecByMatrix4(
        C_STRUCT aiVector3D *vec,
        const C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API void aiMultiplyMatrix4(
        C_STRUCT aiMatrix4x4 *dst,
        const C_STRUCT aiMatrix4x4 *src);



ASSIMP_API void aiMultiplyMatrix3(
        C_STRUCT aiMatrix3x3 *dst,
        const C_STRUCT aiMatrix3x3 *src);



ASSIMP_API void aiIdentityMatrix3(
        C_STRUCT aiMatrix3x3 *mat);



ASSIMP_API void aiIdentityMatrix4(
        C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API size_t aiGetImportFormatCount(void);



ASSIMP_API const C_STRUCT aiImporterDesc *aiGetImportFormatDescription(size_t pIndex);



ASSIMP_API int aiVector2AreEqual(
        const C_STRUCT aiVector2D *a,
        const C_STRUCT aiVector2D *b);



ASSIMP_API int aiVector2AreEqualEpsilon(
        const C_STRUCT aiVector2D *a,
        const C_STRUCT aiVector2D *b,
        const float epsilon);



ASSIMP_API void aiVector2Add(
        C_STRUCT aiVector2D *dst,
        const C_STRUCT aiVector2D *src);



ASSIMP_API void aiVector2Subtract(
        C_STRUCT aiVector2D *dst,
        const C_STRUCT aiVector2D *src);



ASSIMP_API void aiVector2Scale(
        C_STRUCT aiVector2D *dst,
        const float s);



ASSIMP_API void aiVector2SymMul(
        C_STRUCT aiVector2D *dst,
        const C_STRUCT aiVector2D *other);



ASSIMP_API void aiVector2DivideByScalar(
        C_STRUCT aiVector2D *dst,
        const float s);



ASSIMP_API void aiVector2DivideByVector(
        C_STRUCT aiVector2D *dst,
        C_STRUCT aiVector2D *v);



ASSIMP_API ai_real aiVector2Length(
        const C_STRUCT aiVector2D *v);



ASSIMP_API ai_real aiVector2SquareLength(
        const C_STRUCT aiVector2D *v);



ASSIMP_API void aiVector2Negate(
        C_STRUCT aiVector2D *dst);



ASSIMP_API ai_real aiVector2DotProduct(
        const C_STRUCT aiVector2D *a,
        const C_STRUCT aiVector2D *b);



ASSIMP_API void aiVector2Normalize(
        C_STRUCT aiVector2D *v);



ASSIMP_API int aiVector3AreEqual(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);



ASSIMP_API int aiVector3AreEqualEpsilon(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b,
        const float epsilon);



ASSIMP_API int aiVector3LessThan(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);



ASSIMP_API void aiVector3Add(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *src);



ASSIMP_API void aiVector3Subtract(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *src);



ASSIMP_API void aiVector3Scale(
        C_STRUCT aiVector3D *dst,
        const float s);



ASSIMP_API void aiVector3SymMul(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *other);



ASSIMP_API void aiVector3DivideByScalar(
        C_STRUCT aiVector3D *dst,
        const float s);



ASSIMP_API void aiVector3DivideByVector(
        C_STRUCT aiVector3D *dst,
        C_STRUCT aiVector3D *v);



ASSIMP_API ai_real aiVector3Length(
        const C_STRUCT aiVector3D *v);



ASSIMP_API ai_real aiVector3SquareLength(
        const C_STRUCT aiVector3D *v);



ASSIMP_API void aiVector3Negate(
        C_STRUCT aiVector3D *dst);



ASSIMP_API ai_real aiVector3DotProduct(
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);



ASSIMP_API void aiVector3CrossProduct(
        C_STRUCT aiVector3D *dst,
        const C_STRUCT aiVector3D *a,
        const C_STRUCT aiVector3D *b);



ASSIMP_API void aiVector3Normalize(
        C_STRUCT aiVector3D *v);



ASSIMP_API void aiVector3NormalizeSafe(
        C_STRUCT aiVector3D *v);



ASSIMP_API void aiVector3RotateByQuaternion(
        C_STRUCT aiVector3D *v,
        const C_STRUCT aiQuaternion *q);



ASSIMP_API void aiMatrix3FromMatrix4(
        C_STRUCT aiMatrix3x3 *dst,
        const C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API void aiMatrix3FromQuaternion(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiQuaternion *q);



ASSIMP_API int aiMatrix3AreEqual(
        const C_STRUCT aiMatrix3x3 *a,
        const C_STRUCT aiMatrix3x3 *b);



ASSIMP_API int aiMatrix3AreEqualEpsilon(
        const C_STRUCT aiMatrix3x3 *a,
        const C_STRUCT aiMatrix3x3 *b,
        const float epsilon);



ASSIMP_API void aiMatrix3Inverse(
        C_STRUCT aiMatrix3x3 *mat);



ASSIMP_API ai_real aiMatrix3Determinant(
        const C_STRUCT aiMatrix3x3 *mat);



ASSIMP_API void aiMatrix3RotationZ(
        C_STRUCT aiMatrix3x3 *mat,
        const float angle);



ASSIMP_API void aiMatrix3FromRotationAroundAxis(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiVector3D *axis,
        const float angle);



ASSIMP_API void aiMatrix3Translation(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiVector2D *translation);



ASSIMP_API void aiMatrix3FromTo(
        C_STRUCT aiMatrix3x3 *mat,
        const C_STRUCT aiVector3D *from,
        const C_STRUCT aiVector3D *to);



ASSIMP_API void aiMatrix4FromMatrix3(
        C_STRUCT aiMatrix4x4 *dst,
        const C_STRUCT aiMatrix3x3 *mat);



ASSIMP_API void aiMatrix4FromScalingQuaternionPosition(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *scaling,
        const C_STRUCT aiQuaternion *rotation,
        const C_STRUCT aiVector3D *position);



ASSIMP_API void aiMatrix4Add(
        C_STRUCT aiMatrix4x4 *dst,
        const C_STRUCT aiMatrix4x4 *src);



ASSIMP_API int aiMatrix4AreEqual(
        const C_STRUCT aiMatrix4x4 *a,
        const C_STRUCT aiMatrix4x4 *b);



ASSIMP_API int aiMatrix4AreEqualEpsilon(
        const C_STRUCT aiMatrix4x4 *a,
        const C_STRUCT aiMatrix4x4 *b,
        const float epsilon);



ASSIMP_API void aiMatrix4Inverse(
        C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API ai_real aiMatrix4Determinant(
        const C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API int aiMatrix4IsIdentity(
        const C_STRUCT aiMatrix4x4 *mat);



ASSIMP_API void aiMatrix4DecomposeIntoScalingEulerAnglesPosition(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiVector3D *scaling,
        C_STRUCT aiVector3D *rotation,
        C_STRUCT aiVector3D *position);



ASSIMP_API void aiMatrix4DecomposeIntoScalingAxisAnglePosition(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiVector3D *scaling,
        C_STRUCT aiVector3D *axis,
        ai_real *angle,
        C_STRUCT aiVector3D *position);



ASSIMP_API void aiMatrix4DecomposeNoScaling(
        const C_STRUCT aiMatrix4x4 *mat,
        C_STRUCT aiQuaternion *rotation,
        C_STRUCT aiVector3D *position);



ASSIMP_API void aiMatrix4FromEulerAngles(
        C_STRUCT aiMatrix4x4 *mat,
        float x, float y, float z);



ASSIMP_API void aiMatrix4RotationX(
        C_STRUCT aiMatrix4x4 *mat,
        const float angle);



ASSIMP_API void aiMatrix4RotationY(
        C_STRUCT aiMatrix4x4 *mat,
        const float angle);



ASSIMP_API void aiMatrix4RotationZ(
        C_STRUCT aiMatrix4x4 *mat,
        const float angle);



ASSIMP_API void aiMatrix4FromRotationAroundAxis(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *axis,
        const float angle);



ASSIMP_API void aiMatrix4Translation(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *translation);



ASSIMP_API void aiMatrix4Scaling(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *scaling);



ASSIMP_API void aiMatrix4FromTo(
        C_STRUCT aiMatrix4x4 *mat,
        const C_STRUCT aiVector3D *from,
        const C_STRUCT aiVector3D *to);



ASSIMP_API void aiQuaternionFromEulerAngles(
        C_STRUCT aiQuaternion *q,
        float x, float y, float z);



ASSIMP_API void aiQuaternionFromAxisAngle(
        C_STRUCT aiQuaternion *q,
        const C_STRUCT aiVector3D *axis,
        const float angle);



ASSIMP_API void aiQuaternionFromNormalizedQuaternion(
        C_STRUCT aiQuaternion *q,
        const C_STRUCT aiVector3D *normalized);



ASSIMP_API int aiQuaternionAreEqual(
        const C_STRUCT aiQuaternion *a,
        const C_STRUCT aiQuaternion *b);



ASSIMP_API int aiQuaternionAreEqualEpsilon(
        const C_STRUCT aiQuaternion *a,
        const C_STRUCT aiQuaternion *b,
        const float epsilon);



ASSIMP_API void aiQuaternionNormalize(
        C_STRUCT aiQuaternion *q);



ASSIMP_API void aiQuaternionConjugate(
        C_STRUCT aiQuaternion *q);



ASSIMP_API void aiQuaternionMultiply(
        C_STRUCT aiQuaternion *dst,
        const C_STRUCT aiQuaternion *q);



ASSIMP_API void aiQuaternionInterpolate(
        C_STRUCT aiQuaternion *dst,
        const C_STRUCT aiQuaternion *start,
        const C_STRUCT aiQuaternion *end,
        const float factor);

#ifdef __cplusplus
}
#endif

#endif 
