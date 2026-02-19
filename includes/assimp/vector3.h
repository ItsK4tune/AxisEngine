

#pragma once
#ifndef AI_VECTOR3D_H_INC
#define AI_VECTOR3D_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#ifdef __cplusplus
#   include <cmath>
#else
#   include <math.h>
#endif

#include <assimp/defs.h>

#ifdef __cplusplus

template<typename TReal> class aiMatrix3x3t;
template<typename TReal> class aiMatrix4x4t;




template <typename TReal>
class aiVector3t {
public:
    
    aiVector3t() AI_NO_EXCEPT : x(), y(), z() {}

    
    
    
    
    aiVector3t(TReal _x, TReal _y, TReal _z) : x(_x), y(_y), z(_z) {}

    
    
    explicit aiVector3t (TReal _xyz ) : x(_xyz), y(_xyz), z(_xyz) {}

    
    
    aiVector3t( const aiVector3t& o ) = default;

    
    
    const aiVector3t& operator += (const aiVector3t& o);

    
    const aiVector3t& operator -= (const aiVector3t& o);

    
    const aiVector3t& operator *= (TReal f);

    
    const aiVector3t& operator /= (TReal f);

    
    aiVector3t& operator *= (const aiMatrix3x3t<TReal>& mat);
    aiVector3t& operator *= (const aiMatrix4x4t<TReal>& mat);

    
    TReal operator[](unsigned int i) const;

    
    TReal& operator[](unsigned int i);

    
    bool operator== (const aiVector3t& other) const;
    bool operator!= (const aiVector3t& other) const;
    bool operator < (const aiVector3t& other) const;

    
    bool Equal(const aiVector3t &other, TReal epsilon = ai_epsilon) const;

    template <typename TOther>
    operator aiVector3t<TOther> () const;

    
    void Set( TReal pX, TReal pY, TReal pZ);

    
    TReal SquareLength() const;

    
    TReal Length() const;


    
    aiVector3t& Normalize();

    
    aiVector3t& NormalizeSafe();

    
    const aiVector3t SymMul(const aiVector3t& o);

    TReal x, y, z;
};


typedef aiVector3t<ai_real> aiVector3D;
typedef aiVector3t<float> aiVector3f;
typedef aiVector3t<double> aiVector3d;

#else

struct aiVector3D {
    ai_real x, y, z;
};

#endif 

#ifdef __cplusplus

#endif 

#endif 
