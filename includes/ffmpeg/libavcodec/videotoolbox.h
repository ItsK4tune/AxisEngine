

#ifndef AVCODEC_VIDEOTOOLBOX_H
#define AVCODEC_VIDEOTOOLBOX_H





#include <stdint.h>

#define Picture QuickdrawPicture
#include <VideoToolbox/VideoToolbox.h>
#undef Picture

#include "libavcodec/avcodec.h"

#include "libavutil/attributes.h"


typedef struct AVVideotoolboxContext {
    
    VTDecompressionSessionRef session;

    
    OSType cv_pix_fmt_type;

    
    CMVideoFormatDescriptionRef cm_fmt_desc;

    
    int cm_codec_type;
} AVVideotoolboxContext;



#endif 
