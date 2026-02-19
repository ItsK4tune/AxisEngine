


#pragma once
#ifndef AI_MATRIX3X3_H_INC
#define AI_MATRIX3X3_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/defs.h>

#ifdef __cplusplus

template <typename T> class aiMatrix4x4t;
template <typename T> class aiVector2t;
template <typename T> class aiVector3t;



template <typename TReal>
class aiMatrix3x3t {
public:
    aiMatrix3x3t() AI_NO_EXCEPT :
        a1(static_cast<TReal>(1.0f)), a2(), a3(),
        b1(), b2(static_cast<TReal>(1.0f)), b3(),
        c1(), c2(), c3(static_cast<TReal>(1.0f)) {}

    aiMatrix3x3t (  TReal _a1, TReal _a2, TReal _a3,
                    TReal _b1, TReal _b2, TReal _b3,
                    TReal _c1, TReal _c2, TReal _c3) :
        a1(_a1), a2(_a2), a3(_a3),
        b1(_b1), b2(_b2), b3(_b3),
        c1(_c1), c2(_c2), c3(_c3)
    {}

    
    aiMatrix3x3t& operator *= (const aiMatrix3x3t& m);
    aiMatrix3x3t  operator  * (const aiMatrix3x3t& m) const;

    
    TReal* operator[]       (unsigned int p_iIndex);
    const TReal* operator[] (unsigned int p_iIndex) const;

    
    bool operator== (const aiMatrix3x3t<TReal>& m) const;
    bool operator!= (const aiMatrix3x3t<TReal>& m) const;

    bool Equal(const aiMatrix3x3t<TReal> &m, TReal epsilon = ai_epsilon) const;

    template <typename TOther>
    operator aiMatrix3x3t<TOther> () const;

    
    
    explicit aiMatrix3x3t( const aiMatrix4x4t<TReal>& pMatrix);

    
    
    aiMatrix3x3t& Transpose();

    
    
    aiMatrix3x3t& Inverse();
    TReal Determinant() const;

    
    
    static aiMatrix3x3t& RotationZ(TReal a, aiMatrix3x3t& out);

    
    
    static aiMatrix3x3t& Rotation( TReal a, const aiVector3t<TReal>& axis, aiMatrix3x3t& out);

    
    
    static aiMatrix3x3t& Translation( const aiVector2t<TReal>& v, aiMatrix3x3t& out);

    
    
    static aiMatrix3x3t& FromToMatrix(const aiVector3t<TReal>& from,
        const aiVector3t<TReal>& to, aiMatrix3x3t& out);

public:
    TReal a1, a2, a3;
    TReal b1, b2, b3;
    TReal c1, c2, c3;
};

typedef aiMatrix3x3t<ai_real> aiMatrix3x3;

#else

struct aiMatrix3x3 {
    ai_real a1, a2, a3;
    ai_real b1, b2, b3;
    ai_real c1, c2, c3;
};

#endif 

#endif 
