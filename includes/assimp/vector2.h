

#pragma once
#ifndef AI_VECTOR2D_H_INC
#define AI_VECTOR2D_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#ifdef __cplusplus
#   include <cmath>
#else
#   include <math.h>
#endif

#include "defs.h"




#ifdef __cplusplus
template <typename TReal>
class aiVector2t {
public:
    aiVector2t () : x(), y() {}
    aiVector2t (TReal _x, TReal _y) : x(_x), y(_y) {}
    explicit aiVector2t (TReal _xyz) : x(_xyz), y(_xyz) {}
    aiVector2t (const aiVector2t& o) = default;

    void Set( TReal pX, TReal pY);
    TReal SquareLength() const ;
    TReal Length() const ;
    aiVector2t& Normalize();

    const aiVector2t& operator += (const aiVector2t& o);
    const aiVector2t& operator -= (const aiVector2t& o);
    const aiVector2t& operator *= (TReal f);
    const aiVector2t& operator /= (TReal f);

    TReal operator[](unsigned int i) const;

    bool operator== (const aiVector2t& other) const;
    bool operator!= (const aiVector2t& other) const;

    bool Equal(const aiVector2t &other, TReal epsilon = ai_epsilon) const;

    aiVector2t& operator= (TReal f);
    const aiVector2t SymMul(const aiVector2t& o);

    template <typename TOther>
    operator aiVector2t<TOther> () const;

    TReal x, y;
};

typedef aiVector2t<ai_real> aiVector2D;

#else

struct aiVector2D {
    ai_real x, y;
};

#endif 

#endif 
