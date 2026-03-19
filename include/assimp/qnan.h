


#pragma once
#ifndef AI_QNAN_H_INCLUDED
#define AI_QNAN_H_INCLUDED

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/defs.h>

#include <limits>
#include <stdint.h>



union _IEEESingle {
    float Float;
    struct
    {
        uint32_t Frac : 23;
        uint32_t Exp  : 8;
        uint32_t Sign : 1;
    } IEEE;
};



union _IEEEDouble {
    double Double;
    struct
    {
        uint64_t Frac : 52;
        uint64_t Exp  : 11;
        uint64_t Sign : 1;
    } IEEE;
};



AI_FORCE_INLINE bool is_qnan(float in) {
    
    
    
    
    

    
    _IEEESingle temp;
    memcpy(&temp, &in, sizeof(float));
    return (temp.IEEE.Exp == (1u << 8)-1 &&
        temp.IEEE.Frac);
}



AI_FORCE_INLINE bool is_qnan(double in) {
    
    
    
    
    

    
    _IEEEDouble temp;
    memcpy(&temp, &in, sizeof(in));
    return (temp.IEEE.Exp == (1u << 11)-1 &&
        temp.IEEE.Frac);
}



AI_FORCE_INLINE bool is_special_float(float in) {
    _IEEESingle temp;
    memcpy(&temp, &in, sizeof(float));
    return (temp.IEEE.Exp == (1u << 8)-1);
}



AI_FORCE_INLINE bool is_special_float(double in) {
   _IEEESingle temp;
    memcpy(&temp, &in, sizeof(float));
    return (temp.IEEE.Exp == (1u << 11)-1);
}



template<class TReal>
AI_FORCE_INLINE bool is_not_qnan(TReal in) {
    return !is_qnan(in);
}



AI_FORCE_INLINE ai_real get_qnan() {
    return std::numeric_limits<ai_real>::quiet_NaN();
}

#endif 
