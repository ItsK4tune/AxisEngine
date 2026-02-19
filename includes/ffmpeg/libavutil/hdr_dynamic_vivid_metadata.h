

#ifndef AVUTIL_HDR_DYNAMIC_VIVID_METADATA_H
#define AVUTIL_HDR_DYNAMIC_VIVID_METADATA_H

#include "frame.h"
#include "rational.h"


typedef struct AVHDRVivid3SplineParams {
    
    int th_mode;

    
    AVRational th_enable_mb;

    
    AVRational th_enable;

    
    AVRational th_delta1;

    
    AVRational th_delta2;

    
    AVRational enable_strength;
} AVHDRVivid3SplineParams;


typedef struct AVHDRVividColorToneMappingParams {
    
    AVRational targeted_system_display_maximum_luminance;

    
    int base_enable_flag;

    
    AVRational base_param_m_p;

    
    AVRational base_param_m_m;

    
    AVRational base_param_m_a;

    
    AVRational base_param_m_b;

    
    AVRational base_param_m_n;

    
    int base_param_k1;

    
    int base_param_k2;

    
    int base_param_k3;

    
    int base_param_Delta_enable_mode;

    
    AVRational base_param_Delta;

    
    int three_Spline_enable_flag;

    
    int three_Spline_num;

    AVHDRVivid3SplineParams three_spline[2];
} AVHDRVividColorToneMappingParams;



typedef struct AVHDRVividColorTransformParams {
    
    AVRational minimum_maxrgb;

    
    AVRational average_maxrgb;

    
    AVRational variance_maxrgb;

    
    AVRational maximum_maxrgb;

    
    int tone_mapping_mode_flag;

    
    int tone_mapping_param_num;

    
    AVHDRVividColorToneMappingParams tm_params[2];

    
    int color_saturation_mapping_flag;

    
    int color_saturation_num;

    
    AVRational color_saturation_gain[8];
} AVHDRVividColorTransformParams;


typedef struct AVDynamicHDRVivid {
    
    uint8_t system_start_code;

    
    uint8_t num_windows;

    
    AVHDRVividColorTransformParams params[3];
} AVDynamicHDRVivid;


AVDynamicHDRVivid *av_dynamic_hdr_vivid_alloc(size_t *size);


AVDynamicHDRVivid *av_dynamic_hdr_vivid_create_side_data(AVFrame *frame);

#endif 
