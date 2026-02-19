


#pragma once
#ifndef AI_QUATERNION_H_INC
#define AI_QUATERNION_H_INC

#include <assimp/defs.h>

#ifdef __cplusplus

#ifdef __GNUC__
#   pragma GCC system_header
#endif


template <typename TReal> class aiVector3t;
template <typename TReal> class aiMatrix3x3t;
template <typename TReal> class aiMatrix4x4t;



template <typename TReal>
class aiQuaterniont {
public:
    aiQuaterniont() AI_NO_EXCEPT : w(1.0), x(), y(), z() {}
    aiQuaterniont(TReal pw, TReal px, TReal py, TReal pz)
        : w(pw), x(px), y(py), z(pz) {}

    
    explicit aiQuaterniont( const aiMatrix3x3t<TReal>& pRotMatrix);

    
    aiQuaterniont( TReal roty, TReal rotz, TReal rotx);

    
    aiQuaterniont( aiVector3t<TReal> axis, TReal angle);

    
    explicit aiQuaterniont( aiVector3t<TReal> normalized);

    
    aiMatrix3x3t<TReal> GetMatrix() const;

    bool operator== (const aiQuaterniont& o) const;
    bool operator!= (const aiQuaterniont& o) const;

    
    aiQuaterniont& operator *= (const aiMatrix4x4t<TReal>& mat);

    bool Equal(const aiQuaterniont &o, TReal epsilon = ai_epsilon) const;

    
    aiQuaterniont& Normalize();

    
    aiQuaterniont& Conjugate();

    
    aiVector3t<TReal> Rotate(const aiVector3t<TReal>& in) const;

    
    aiQuaterniont operator * (const aiQuaterniont& two) const;

    
    static void Interpolate( aiQuaterniont& pOut, const aiQuaterniont& pStart,
        const aiQuaterniont& pEnd, TReal pFactor);

    
    TReal w, x, y, z;
} ;

using aiQuaternion = aiQuaterniont<ai_real>;

#else

struct aiQuaternion {
    ai_real w, x, y, z;
};

#endif

#endif 
