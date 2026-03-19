

#ifndef AVCODEC_D3D11VA_H
#define AVCODEC_D3D11VA_H



#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif

#include <stdint.h>
#include <d3d11.h>




typedef struct AVD3D11VAContext {
    
    ID3D11VideoDecoder *decoder;

    
    ID3D11VideoContext *video_context;

    
    D3D11_VIDEO_DECODER_CONFIG *cfg;

    
    unsigned surface_count;

    
    ID3D11VideoDecoderOutputView **surface;

    
    uint64_t workaround;

    
    unsigned report_id;

    
    HANDLE  context_mutex;
} AVD3D11VAContext;


AVD3D11VAContext *av_d3d11va_alloc_context(void);



#endif 
