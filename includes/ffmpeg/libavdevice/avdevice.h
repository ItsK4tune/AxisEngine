

#ifndef AVDEVICE_AVDEVICE_H
#define AVDEVICE_AVDEVICE_H

#include "version_major.h"
#ifndef HAVE_AV_CONFIG_H

#include "version.h"
#endif





#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavformat/avformat.h"


unsigned avdevice_version(void);


const char *avdevice_configuration(void);


const char *avdevice_license(void);


void avdevice_register_all(void);


const AVInputFormat *av_input_audio_device_next(const AVInputFormat  *d);


const AVInputFormat *av_input_video_device_next(const AVInputFormat  *d);


const AVOutputFormat *av_output_audio_device_next(const AVOutputFormat *d);


const AVOutputFormat *av_output_video_device_next(const AVOutputFormat *d);

typedef struct AVDeviceRect {
    int x;      
    int y;      
    int width;  
    int height; 
} AVDeviceRect;


enum AVAppToDevMessageType {
    
    AV_APP_TO_DEV_NONE = MKBETAG('N','O','N','E'),

    
    AV_APP_TO_DEV_WINDOW_SIZE = MKBETAG('G','E','O','M'),

    
    AV_APP_TO_DEV_WINDOW_REPAINT = MKBETAG('R','E','P','A'),

    
    AV_APP_TO_DEV_PAUSE        = MKBETAG('P', 'A', 'U', ' '),
    AV_APP_TO_DEV_PLAY         = MKBETAG('P', 'L', 'A', 'Y'),
    AV_APP_TO_DEV_TOGGLE_PAUSE = MKBETAG('P', 'A', 'U', 'T'),

    
    AV_APP_TO_DEV_SET_VOLUME = MKBETAG('S', 'V', 'O', 'L'),

    
    AV_APP_TO_DEV_MUTE        = MKBETAG(' ', 'M', 'U', 'T'),
    AV_APP_TO_DEV_UNMUTE      = MKBETAG('U', 'M', 'U', 'T'),
    AV_APP_TO_DEV_TOGGLE_MUTE = MKBETAG('T', 'M', 'U', 'T'),

    
    AV_APP_TO_DEV_GET_VOLUME = MKBETAG('G', 'V', 'O', 'L'),
    AV_APP_TO_DEV_GET_MUTE   = MKBETAG('G', 'M', 'U', 'T'),
};


enum AVDevToAppMessageType {
    
    AV_DEV_TO_APP_NONE = MKBETAG('N','O','N','E'),

    
    AV_DEV_TO_APP_CREATE_WINDOW_BUFFER = MKBETAG('B','C','R','E'),

    
    AV_DEV_TO_APP_PREPARE_WINDOW_BUFFER = MKBETAG('B','P','R','E'),

    
    AV_DEV_TO_APP_DISPLAY_WINDOW_BUFFER = MKBETAG('B','D','I','S'),

    
    AV_DEV_TO_APP_DESTROY_WINDOW_BUFFER = MKBETAG('B','D','E','S'),

    
    AV_DEV_TO_APP_BUFFER_OVERFLOW = MKBETAG('B','O','F','L'),
    AV_DEV_TO_APP_BUFFER_UNDERFLOW = MKBETAG('B','U','F','L'),

    
    AV_DEV_TO_APP_BUFFER_READABLE = MKBETAG('B','R','D',' '),
    AV_DEV_TO_APP_BUFFER_WRITABLE = MKBETAG('B','W','R',' '),

    
    AV_DEV_TO_APP_MUTE_STATE_CHANGED = MKBETAG('C','M','U','T'),

    
    AV_DEV_TO_APP_VOLUME_LEVEL_CHANGED = MKBETAG('C','V','O','L'),
};


int avdevice_app_to_dev_control_message(struct AVFormatContext *s,
                                        enum AVAppToDevMessageType type,
                                        void *data, size_t data_size);


int avdevice_dev_to_app_control_message(struct AVFormatContext *s,
                                        enum AVDevToAppMessageType type,
                                        void *data, size_t data_size);


typedef struct AVDeviceInfo {
    char *device_name;                   
    char *device_description;            
    enum AVMediaType *media_types;       
    int nb_media_types;                  
} AVDeviceInfo;


typedef struct AVDeviceInfoList {
    AVDeviceInfo **devices;              
    int nb_devices;                      
    int default_device;                  
} AVDeviceInfoList;


int avdevice_list_devices(struct AVFormatContext *s, AVDeviceInfoList **device_list);


void avdevice_free_list_devices(AVDeviceInfoList **device_list);


int avdevice_list_input_sources(const AVInputFormat *device, const char *device_name,
                                AVDictionary *device_options, AVDeviceInfoList **device_list);
int avdevice_list_output_sinks(const AVOutputFormat *device, const char *device_name,
                               AVDictionary *device_options, AVDeviceInfoList **device_list);



#endif 
