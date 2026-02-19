

#ifndef AVCODEC_DXVA2_H
#define AVCODEC_DXVA2_H



#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif

#include <stdint.h>
#include <d3d9.h>
#include <dxva2api.h>




struct dxva_context {
    
    IDirectXVideoDecoder *decoder;

    
    const DXVA2_ConfigPictureDecode *cfg;

    
    unsigned surface_count;

    
    LPDIRECT3DSURFACE9 *surface;

    
    uint64_t workaround;

    
    unsigned report_id;
};



#endif 
