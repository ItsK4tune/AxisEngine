

#pragma once
#ifndef AI_COLOR4D_H_INC
#define AI_COLOR4D_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/defs.h>

#ifdef __cplusplus




template <typename TReal>
class aiColor4t {
public:
    aiColor4t() AI_NO_EXCEPT : r(), g(), b(), a() {}
    aiColor4t (TReal _r, TReal _g, TReal _b, TReal _a)
        : r(_r), g(_g), b(_b), a(_a) {}
    explicit aiColor4t (TReal _r) : r(_r), g(_r), b(_r), a(_r) {}
    aiColor4t (const aiColor4t& o) = default;

    
    const aiColor4t& operator += (const aiColor4t& o);
    const aiColor4t& operator -= (const aiColor4t& o);
    const aiColor4t& operator *= (TReal f);
    const aiColor4t& operator /= (TReal f);

    
    bool operator == (const aiColor4t& other) const;
    bool operator != (const aiColor4t& other) const;
    bool operator <  (const aiColor4t& other) const;

    
    inline TReal operator[](unsigned int i) const;
    inline TReal& operator[](unsigned int i);

    
    inline bool IsBlack() const;

    
    TReal r, g, b, a;
};  

typedef aiColor4t<float> aiColor4D;

#else

struct aiColor4D {
    float r, g, b, a;
};

#endif 

#endif 
