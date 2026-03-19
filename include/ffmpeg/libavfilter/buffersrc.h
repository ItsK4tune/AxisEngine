

#ifndef AVFILTER_BUFFERSRC_H
#define AVFILTER_BUFFERSRC_H



#include "avfilter.h"



enum {

    
    AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT = 1,

    
    AV_BUFFERSRC_FLAG_PUSH = 4,

    
    AV_BUFFERSRC_FLAG_KEEP_REF = 8,

};


unsigned av_buffersrc_get_nb_failed_requests(AVFilterContext *buffer_src);


typedef struct AVBufferSrcParameters {
    
    int format;
    
    AVRational time_base;

    
    int width, height;

    
    AVRational sample_aspect_ratio;

    
    AVRational frame_rate;

    
    AVBufferRef *hw_frames_ctx;

    
    int sample_rate;

    
    AVChannelLayout ch_layout;

    
    enum AVColorSpace color_space;
    enum AVColorRange color_range;

    AVFrameSideData **side_data;
    int nb_side_data;
} AVBufferSrcParameters;


AVBufferSrcParameters *av_buffersrc_parameters_alloc(void);


int av_buffersrc_parameters_set(AVFilterContext *ctx, AVBufferSrcParameters *param);


av_warn_unused_result
int av_buffersrc_write_frame(AVFilterContext *ctx, const AVFrame *frame);


av_warn_unused_result
int av_buffersrc_add_frame(AVFilterContext *ctx, AVFrame *frame);


av_warn_unused_result
int av_buffersrc_add_frame_flags(AVFilterContext *buffer_src,
                                 AVFrame *frame, int flags);


int av_buffersrc_close(AVFilterContext *ctx, int64_t pts, unsigned flags);



#endif 
