

#ifndef AVUTIL_AMBIENT_VIEWING_ENVIRONMENT_H
#define AVUTIL_AMBIENT_VIEWING_ENVIRONMENT_H

#include <stddef.h>
#include "frame.h"
#include "rational.h"


typedef struct AVAmbientViewingEnvironment {
    
    AVRational ambient_illuminance;

    
    AVRational ambient_light_x;

    
    AVRational ambient_light_y;
} AVAmbientViewingEnvironment;


AVAmbientViewingEnvironment *av_ambient_viewing_environment_alloc(size_t *size);


AVAmbientViewingEnvironment *av_ambient_viewing_environment_create_side_data(AVFrame *frame);

#endif 
