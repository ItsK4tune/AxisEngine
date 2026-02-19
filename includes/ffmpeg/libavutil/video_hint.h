

#ifndef AVUTIL_VIDEO_HINT_H
#define AVUTIL_VIDEO_HINT_H

#include <stddef.h>
#include <stdint.h>
#include "libavutil/avassert.h"
#include "libavutil/frame.h"

typedef struct AVVideoRect {
    uint32_t x, y;
    uint32_t width, height;
} AVVideoRect;

typedef enum AVVideoHintType {
    
    AV_VIDEO_HINT_TYPE_CONSTANT,

    
    AV_VIDEO_HINT_TYPE_CHANGED,
} AVVideoHintType;

typedef struct AVVideoHint {
    
    size_t nb_rects;

    
    size_t rect_offset;

    
    size_t rect_size;

    AVVideoHintType type;
} AVVideoHint;

static av_always_inline AVVideoRect *
av_video_hint_rects(const AVVideoHint *hints) {
    return (AVVideoRect *)((uint8_t *)hints + hints->rect_offset);
}

static av_always_inline AVVideoRect *
av_video_hint_get_rect(const AVVideoHint *hints, size_t idx) {
    return (AVVideoRect *)((uint8_t *)hints + hints->rect_offset + idx * hints->rect_size);
}


AVVideoHint *av_video_hint_alloc(size_t nb_rects,
                                 size_t *out_size);


AVVideoHint *av_video_hint_create_side_data(AVFrame *frame,
                                            size_t nb_rects);


#endif 
