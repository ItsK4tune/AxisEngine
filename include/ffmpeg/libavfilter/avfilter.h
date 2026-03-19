

#ifndef AVFILTER_AVFILTER_H
#define AVFILTER_AVFILTER_H





#include <stddef.h>

#include "libavutil/attributes.h"
#include "libavutil/avutil.h"
#include "libavutil/buffer.h"
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavutil/log.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"

#include "libavfilter/version_major.h"
#ifndef HAVE_AV_CONFIG_H

#include "libavfilter/version.h"
#endif


unsigned avfilter_version(void);


const char *avfilter_configuration(void);


const char *avfilter_license(void);

typedef struct AVFilterLink    AVFilterLink;
typedef struct AVFilterPad     AVFilterPad;
typedef struct AVFilterFormats AVFilterFormats;
typedef struct AVFilterChannelLayouts AVFilterChannelLayouts;


const char *avfilter_pad_get_name(const AVFilterPad *pads, int pad_idx);


enum AVMediaType avfilter_pad_get_type(const AVFilterPad *pads, int pad_idx);


AVBufferRef* avfilter_link_get_hw_frames_ctx(AVFilterLink *link);


typedef struct AVFilterFormatsConfig {

    
    AVFilterFormats *formats;

    
    AVFilterFormats  *samplerates;

    
    AVFilterChannelLayouts  *channel_layouts;

    
    AVFilterFormats *color_spaces;  
    AVFilterFormats *color_ranges;  

} AVFilterFormatsConfig;


#define AVFILTER_FLAG_DYNAMIC_INPUTS        (1 << 0)

#define AVFILTER_FLAG_DYNAMIC_OUTPUTS       (1 << 1)

#define AVFILTER_FLAG_SLICE_THREADS         (1 << 2)

#define AVFILTER_FLAG_METADATA_ONLY         (1 << 3)


#define AVFILTER_FLAG_HWDEVICE              (1 << 4)

#define AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC  (1 << 16)

#define AVFILTER_FLAG_SUPPORT_TIMELINE_INTERNAL (1 << 17)

#define AVFILTER_FLAG_SUPPORT_TIMELINE (AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC | AVFILTER_FLAG_SUPPORT_TIMELINE_INTERNAL)


typedef struct AVFilter {
    
    const char *name;

    
    const char *description;

    
    const AVFilterPad *inputs;

    
    const AVFilterPad *outputs;

    
    const AVClass *priv_class;

    
    int flags;
} AVFilter;


unsigned avfilter_filter_pad_count(const AVFilter *filter, int is_output);


#define AVFILTER_THREAD_SLICE (1 << 0)


typedef struct AVFilterContext {
    const AVClass *av_class;        

    const AVFilter *filter;         

    char *name;                     

    AVFilterPad   *input_pads;      
    AVFilterLink **inputs;          
    unsigned    nb_inputs;          

    AVFilterPad   *output_pads;     
    AVFilterLink **outputs;         
    unsigned    nb_outputs;         

    void *priv;                     

    struct AVFilterGraph *graph;    

    
    int thread_type;

    
    int nb_threads;

#if FF_API_CONTEXT_PUBLIC
    
    attribute_deprecated
    struct AVFilterCommand *command_queue;
#endif

    char *enable_str;               
#if FF_API_CONTEXT_PUBLIC
    
    attribute_deprecated
    void *enable;
    
    double *var_values;
#endif
    
    int is_disabled;

    
    AVBufferRef *hw_device_ctx;

#if FF_API_CONTEXT_PUBLIC
    
    attribute_deprecated
    unsigned ready;
#endif

    
    int extra_hw_frames;
} AVFilterContext;


struct AVFilterLink {
    AVFilterContext *src;       
    AVFilterPad *srcpad;        

    AVFilterContext *dst;       
    AVFilterPad *dstpad;        

    enum AVMediaType type;      

    int format;                 

    
    int w;                      
    int h;                      
    AVRational sample_aspect_ratio; 
    
    enum AVColorSpace colorspace;   
    enum AVColorRange color_range;  

    
    int sample_rate;            
    AVChannelLayout ch_layout;  

    
    AVRational time_base;

    AVFrameSideData **side_data;
    int nb_side_data;

    

    
    AVFilterFormatsConfig incfg;

    
    AVFilterFormatsConfig outcfg;
};


int avfilter_link(AVFilterContext *src, unsigned srcpad,
                  AVFilterContext *dst, unsigned dstpad);

#define AVFILTER_CMD_FLAG_ONE   1 
#define AVFILTER_CMD_FLAG_FAST  2 


int avfilter_process_command(AVFilterContext *filter, const char *cmd, const char *arg, char *res, int res_len, int flags);


const AVFilter *av_filter_iterate(void **opaque);


const AVFilter *avfilter_get_by_name(const char *name);



int avfilter_init_str(AVFilterContext *ctx, const char *args);


int avfilter_init_dict(AVFilterContext *ctx, AVDictionary **options);


void avfilter_free(AVFilterContext *filter);


int avfilter_insert_filter(AVFilterLink *link, AVFilterContext *filt,
                           unsigned filt_srcpad_idx, unsigned filt_dstpad_idx);


const AVClass *avfilter_get_class(void);


typedef int (avfilter_action_func)(AVFilterContext *ctx, void *arg, int jobnr, int nb_jobs);


typedef int (avfilter_execute_func)(AVFilterContext *ctx, avfilter_action_func *func,
                                    void *arg, int *ret, int nb_jobs);

typedef struct AVFilterGraph {
    const AVClass *av_class;
    AVFilterContext **filters;
    unsigned nb_filters;

    char *scale_sws_opts; 

    
    int thread_type;

    
    int nb_threads;

    
    void *opaque;

    
    avfilter_execute_func *execute;

    char *aresample_swr_opts; 

    
    unsigned max_buffered_frames;
} AVFilterGraph;


AVFilterGraph *avfilter_graph_alloc(void);


AVFilterContext *avfilter_graph_alloc_filter(AVFilterGraph *graph,
                                             const AVFilter *filter,
                                             const char *name);


AVFilterContext *avfilter_graph_get_filter(AVFilterGraph *graph, const char *name);


int avfilter_graph_create_filter(AVFilterContext **filt_ctx, const AVFilter *filt,
                                 const char *name, const char *args, void *opaque,
                                 AVFilterGraph *graph_ctx);


void avfilter_graph_set_auto_convert(AVFilterGraph *graph, unsigned flags);

enum {
    AVFILTER_AUTO_CONVERT_ALL  =  0, 
    AVFILTER_AUTO_CONVERT_NONE = -1, 
};


int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx);


void avfilter_graph_free(AVFilterGraph **graph);


typedef struct AVFilterInOut {
    
    char *name;

    
    AVFilterContext *filter_ctx;

    
    int pad_idx;

    
    struct AVFilterInOut *next;
} AVFilterInOut;


AVFilterInOut *avfilter_inout_alloc(void);


void avfilter_inout_free(AVFilterInOut **inout);


int avfilter_graph_parse(AVFilterGraph *graph, const char *filters,
                         AVFilterInOut *inputs, AVFilterInOut *outputs,
                         void *log_ctx);


int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters,
                             AVFilterInOut **inputs, AVFilterInOut **outputs,
                             void *log_ctx);


int avfilter_graph_parse2(AVFilterGraph *graph, const char *filters,
                          AVFilterInOut **inputs,
                          AVFilterInOut **outputs);


typedef struct AVFilterPadParams {
    
    char *label;
} AVFilterPadParams;


typedef struct AVFilterParams {
    
    AVFilterContext     *filter;

    
    char                *filter_name;
    
    char                *instance_name;

    
    AVDictionary        *opts;

    AVFilterPadParams  **inputs;
    unsigned          nb_inputs;

    AVFilterPadParams  **outputs;
    unsigned          nb_outputs;
} AVFilterParams;


typedef struct AVFilterChain {
    AVFilterParams  **filters;
    size_t         nb_filters;
} AVFilterChain;


typedef struct AVFilterGraphSegment {
    
    AVFilterGraph *graph;

    
    AVFilterChain **chains;
    size_t       nb_chains;

    
    char *scale_sws_opts;
} AVFilterGraphSegment;


int avfilter_graph_segment_parse(AVFilterGraph *graph, const char *graph_str,
                                 int flags, AVFilterGraphSegment **seg);


int avfilter_graph_segment_create_filters(AVFilterGraphSegment *seg, int flags);


int avfilter_graph_segment_apply_opts(AVFilterGraphSegment *seg, int flags);


int avfilter_graph_segment_init(AVFilterGraphSegment *seg, int flags);


int avfilter_graph_segment_link(AVFilterGraphSegment *seg, int flags,
                                AVFilterInOut **inputs,
                                AVFilterInOut **outputs);


int avfilter_graph_segment_apply(AVFilterGraphSegment *seg, int flags,
                                 AVFilterInOut **inputs,
                                 AVFilterInOut **outputs);


void avfilter_graph_segment_free(AVFilterGraphSegment **seg);


int avfilter_graph_send_command(AVFilterGraph *graph, const char *target, const char *cmd, const char *arg, char *res, int res_len, int flags);


int avfilter_graph_queue_command(AVFilterGraph *graph, const char *target, const char *cmd, const char *arg, int flags, double ts);



char *avfilter_graph_dump(AVFilterGraph *graph, const char *options);


int avfilter_graph_request_oldest(AVFilterGraph *graph);



#endif 
