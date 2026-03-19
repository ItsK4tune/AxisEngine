

#ifndef AVUTIL_MASTERING_DISPLAY_METADATA_H
#define AVUTIL_MASTERING_DISPLAY_METADATA_H

#include "frame.h"
#include "rational.h"



typedef struct AVMasteringDisplayMetadata {
    
    AVRational display_primaries[3][2];

    
    AVRational white_point[2];

    
    AVRational min_luminance;

    
    AVRational max_luminance;

    
    int has_primaries;

    
    int has_luminance;

} AVMasteringDisplayMetadata;


AVMasteringDisplayMetadata *av_mastering_display_metadata_alloc(void);


AVMasteringDisplayMetadata *av_mastering_display_metadata_alloc_size(size_t *size);


AVMasteringDisplayMetadata *av_mastering_display_metadata_create_side_data(AVFrame *frame);


typedef struct AVContentLightMetadata {
    
    unsigned MaxCLL;

    
    unsigned MaxFALL;
} AVContentLightMetadata;


AVContentLightMetadata *av_content_light_metadata_alloc(size_t *size);


AVContentLightMetadata *av_content_light_metadata_create_side_data(AVFrame *frame);

#endif 
