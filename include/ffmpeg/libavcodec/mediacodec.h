

#ifndef AVCODEC_MEDIACODEC_H
#define AVCODEC_MEDIACODEC_H

#include "libavcodec/avcodec.h"


typedef struct AVMediaCodecContext {

    
    void *surface;

} AVMediaCodecContext;


AVMediaCodecContext *av_mediacodec_alloc_context(void);


int av_mediacodec_default_init(AVCodecContext *avctx, AVMediaCodecContext *ctx, void *surface);


void av_mediacodec_default_free(AVCodecContext *avctx);


typedef struct MediaCodecBuffer AVMediaCodecBuffer;


int av_mediacodec_release_buffer(AVMediaCodecBuffer *buffer, int render);


int av_mediacodec_render_buffer_at_time(AVMediaCodecBuffer *buffer, int64_t time);

#endif 
