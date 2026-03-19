

#ifndef AVCODEC_VDPAU_H
#define AVCODEC_VDPAU_H






#include <vdpau/vdpau.h>

#include "libavutil/avconfig.h"
#include "libavutil/attributes.h"

#include "avcodec.h"

struct AVCodecContext;
struct AVFrame;

typedef int (*AVVDPAU_Render2)(struct AVCodecContext *, struct AVFrame *,
                               const VdpPictureInfo *, uint32_t,
                               const VdpBitstreamBuffer *);


typedef struct AVVDPAUContext {
    
    VdpDecoder decoder;

    
    VdpDecoderRender *render;

    AVVDPAU_Render2 render2;
} AVVDPAUContext;


int av_vdpau_bind_context(AVCodecContext *avctx, VdpDevice device,
                          VdpGetProcAddress *get_proc_address, unsigned flags);


int av_vdpau_get_surface_parameters(AVCodecContext *avctx, VdpChromaType *type,
                                    uint32_t *width, uint32_t *height);



#endif 
