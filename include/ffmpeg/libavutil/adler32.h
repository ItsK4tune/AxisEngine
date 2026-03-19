



#ifndef AVUTIL_ADLER32_H
#define AVUTIL_ADLER32_H

#include <stddef.h>
#include <stdint.h>
#include "attributes.h"



typedef uint32_t AVAdler;


AVAdler av_adler32_update(AVAdler adler, const uint8_t *buf,
                          size_t len) av_pure;



#endif 
