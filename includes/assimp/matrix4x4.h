

#pragma once
#ifndef AI_MATRIX4X4_H_INC
#define AI_MATRIX4X4_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/vector3.h>
#include <assimp/defs.h>
#include <assimp/config.h>

#ifdef __cplusplus

template<typename TReal> class aiMatrix3x3t;
template<typename TReal> class aiQuaterniont;



template<typename TReal>
class aiMatrix4x4t {
public:
    
    aiMatrix4x4t() AI_NO_EXCEPT;

    
    aiMatrix4x4t (  TReal _a1, TReal _a2, TReal _a3, TReal _a4,
                    TReal _b1, TReal _b2, TReal _b3, TReal _b4,
                    TReal _c1, TReal _c2, TReal _c3, TReal _c4,
                    TReal _d1, TReal _d2, TReal _d3, TReal _d4);


    
    explicit aiMatrix4x4t( const aiMatrix3x3t<TReal>& m);

    
    aiMatrix4x4t(const aiVector3t<TReal>& scaling, const aiQuaterniont<TReal>& rotation,
          const aiVector3t<TReal>& position);

    
    
      TReal* operator[] (unsigned int p_iIndex);

    
    const TReal* operator[] (unsigned int p_iIndex) const;

    
    bool operator== (const aiMatrix4x4t& m) const;
    bool operator!= (const aiMatrix4x4t& m) const;

    bool Equal(const aiMatrix4x4t &m, TReal epsilon = ai_epsilon) const;

    
    aiMatrix4x4t& operator *= (const aiMatrix4x4t& m);
    aiMatrix4x4t  operator *  (const aiMatrix4x4t& m) const;
    aiMatrix4x4t operator * (const TReal& aFloat) const;
    aiMatrix4x4t operator + (const aiMatrix4x4t& aMatrix) const;

    template <typename TOther>
    operator aiMatrix4x4t<TOther> () const;

    
    
    aiMatrix4x4t& Transpose();

    
    
    aiMatrix4x4t& Inverse();

    
    
    TReal Determinant() const;

    
    
    inline bool IsIdentity(const TReal
            epsilon = AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT) const;

    
    
    void Decompose (aiVector3t<TReal>& scaling, aiQuaterniont<TReal>& rotation,
        aiVector3t<TReal>& position) const;

    
    
    void Decompose(aiVector3t<TReal>& pScaling, aiVector3t<TReal>& pRotation, aiVector3t<TReal>& pPosition) const;

    
    
    void Decompose(aiVector3t<TReal>& pScaling, aiVector3t<TReal>& pRotationAxis, TReal& pRotationAngle, aiVector3t<TReal>& pPosition) const;

    
    
    void DecomposeNoScaling (aiQuaterniont<TReal>& rotation,
        aiVector3t<TReal>& position) const;

    
    
    aiMatrix4x4t& FromEulerAnglesXYZ(TReal x, TReal y, TReal z);
    aiMatrix4x4t& FromEulerAnglesXYZ(const aiVector3t<TReal>& blubb);

    
    
    static aiMatrix4x4t& RotationX(TReal a, aiMatrix4x4t& out);

    
    
    static aiMatrix4x4t& RotationY(TReal a, aiMatrix4x4t& out);

    
    
    static aiMatrix4x4t& RotationZ(TReal a, aiMatrix4x4t& out);

    
    
    static aiMatrix4x4t& Rotation(TReal a, const aiVector3t<TReal>& axis,
            aiMatrix4x4t& out);

    
    
    static aiMatrix4x4t& Translation( const aiVector3t<TReal>& v,
            aiMatrix4x4t& out);

    
    
    static aiMatrix4x4t& Scaling( const aiVector3t<TReal>& v, aiMatrix4x4t& out);

    
    
    static aiMatrix4x4t& FromToMatrix(const aiVector3t<TReal>& from,
            const aiVector3t<TReal>& to, aiMatrix4x4t& out);

    TReal a1, a2, a3, a4;
    TReal b1, b2, b3, b4;
    TReal c1, c2, c3, c4;
    TReal d1, d2, d3, d4;
};

typedef aiMatrix4x4t<ai_real> aiMatrix4x4;

#else

struct aiMatrix4x4 {
    ai_real a1, a2, a3, a4;
    ai_real b1, b2, b3, b4;
    ai_real c1, c2, c3, c4;
    ai_real d1, d2, d3, d4;
};


#endif 

#endif 
