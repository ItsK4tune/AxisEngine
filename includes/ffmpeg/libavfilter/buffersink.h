

#ifndef AVFILTER_BUFFERSINK_H
#define AVFILTER_BUFFERSINK_H



#include "avfilter.h"




int av_buffersink_get_frame_flags(AVFilterContext *ctx, AVFrame *frame, int flags);


#define AV_BUFFERSINK_FLAG_PEEK 1


#define AV_BUFFERSINK_FLAG_NO_REQUEST 2


void av_buffersink_set_frame_size(AVFilterContext *ctx, unsigned frame_size);



enum AVMediaType av_buffersink_get_type                (const AVFilterContext *ctx);
AVRational       av_buffersink_get_time_base           (const AVFilterContext *ctx);
int              av_buffersink_get_format              (const AVFilterContext *ctx);

AVRational       av_buffersink_get_frame_rate          (const AVFilterContext *ctx);
int              av_buffersink_get_w                   (const AVFilterContext *ctx);
int              av_buffersink_get_h                   (const AVFilterContext *ctx);
AVRational       av_buffersink_get_sample_aspect_ratio (const AVFilterContext *ctx);
enum AVColorSpace av_buffersink_get_colorspace         (const AVFilterContext *ctx);
enum AVColorRange av_buffersink_get_color_range        (const AVFilterContext *ctx);

int              av_buffersink_get_channels            (const AVFilterContext *ctx);
int              av_buffersink_get_ch_layout           (const AVFilterContext *ctx,
                                                        AVChannelLayout *ch_layout);
int              av_buffersink_get_sample_rate         (const AVFilterContext *ctx);

AVBufferRef *    av_buffersink_get_hw_frames_ctx       (const AVFilterContext *ctx);

const AVFrameSideData *const *av_buffersink_get_side_data(const AVFilterContext *ctx,
                                                          int *nb_side_data);




int av_buffersink_get_frame(AVFilterContext *ctx, AVFrame *frame);


int av_buffersink_get_samples(AVFilterContext *ctx, AVFrame *frame, int nb_samples);



#endif 
